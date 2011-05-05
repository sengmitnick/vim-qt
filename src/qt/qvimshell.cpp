#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}

QHash<QString, QColor> QVimShell::m_colorTable;

QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QWidget(parent), m_foreground(Qt::black), m_gui(gui), m_encoding_utf8(true),
	m_input(false), m_lastClickEvent(-1), m_special(QBrush())
{
	setAttribute(Qt::WA_KeyCompression, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void QVimShell::setBackground(const QColor color)
{
	m_background = color;
}

void QVimShell::switchTab(int idx)
{
	send_tabline_event(idx);
	m_input = true;
}

void QVimShell::closeTab(int idx)
{
	send_tabline_menu_event(idx, TABLINE_MENU_CLOSE);
	m_input = true;
}


QColor QVimShell::background()
{
	return m_background;
}

void QVimShell::setForeground(const QColor& color)
{
	m_foreground = color;
}

QColor QVimShell::foreground()
{
	return m_foreground;
}


void QVimShell::setSpecial(const QColor& color)
{
	m_special = QBrush(color);
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	if ( canvas.isNull() ) {
		QPixmap newCanvas = QPixmap( ev->size() );
		newCanvas.fill(m_background);
		canvas = newCanvas;
	} else {
		// Keep old contents
		QPixmap old = canvas.copy(QRect(QPoint(0,0), ev->size()));
		canvas = QPixmap( ev->size() );

		{
		QPainter p(&canvas);
		p.drawPixmap(QPoint(0,0), old);
		}
	}

	update();
	gui_resize_shell(ev->size().width(), ev->size().height());
}


unsigned int QVimShell::vimModifiers(Qt::KeyboardModifiers mod)
{
	if ( mod == Qt::NoModifier ) {
		return 0;
	}
	unsigned int vmod;
	if ( mod & Qt::ShiftModifier ) {
		vmod |= MOD_MASK_SHIFT;
	} if ( mod & Qt::ControlModifier ) {
		vmod |= MOD_MASK_CTRL;
	} if ( mod & Qt::AltModifier ) {
		vmod |= MOD_MASK_ALT;
	} if ( mod & Qt::MetaModifier ) {
		vmod |= MOD_MASK_META;
	}

	return vmod;
}

bool QVimShell::specialKey(int k, char str[3])
{
	int i;
	for ( i=0; special_keys[i].key_sym != 0; i++ ) {
		if ( special_keys[i].key_sym == k ) {
			str[0] = CSI;
			str[1] = special_keys[i].code0;
			str[2] = special_keys[i].code1;
			return true;
		}
	}
	return false;
}

// FIXME
QByteArray QVimShell::convertTo(const QString& s)
{
	if ( m_encoding_utf8 ) {
		return s.toUtf8();
	} else {
		return s.toAscii();
	}
}

QString QVimShell::convertFrom(const char *s, int size)
{
	if ( m_encoding_utf8 ) {
		return QString::fromUtf8(s, size);
	} else {
		return QString::fromAscii(s, size);
	}
}


bool QVimShell::hasInput()
{
	if (m_input) {
		m_input = false;
		return true;
	}
	return false;
}

void QVimShell::forceInput()
{
	m_input = true;
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	char str[3];

	m_input = true;
	if ( specialKey( ev->key(), str)) {
		add_to_input_buf((char_u *) str, 3);
		return;
	}

	if ( !ev->text().isEmpty() ) {
		add_to_input_buf( (char_u *) convertTo(ev->text()).data(), ev->count() );
		return;
	}
}

void QVimShell::closeEvent(QCloseEvent *event)
{
	gui_shell_closed();
	event->ignore();
}

void QVimShell::flushPaintOps()
{
	QPainter painter(&canvas);
	while ( !paintOps.isEmpty() ) {

		PaintOperation op = paintOps.dequeue();
		switch( op.type ) {
		case CLEARALL:
			painter.fillRect(canvas.rect(), op.color);
			break;
		case FILLRECT:
			painter.fillRect(op.rect, op.color);
			break;
		case DRAWRECT:
			painter.drawRect(op.rect); // FIXME: need color
			break;
		case DRAWSTRING:
			painter.setPen( op.color );
			painter.setFont( op.font );
			painter.drawText(op.rect, op.str);
			break;
		case DRAWUNDERCURL:
			painter.setPen(QPen(op.color, 0, Qt::DashLine));
			painter.drawLine( op.rect.right(), op.rect.bottom(), op.rect.left(), op.rect.bottom());
			break;
		case INVERTRECT:
			painter.setCompositionMode( QPainter::RasterOp_SourceXorDestination );
			painter.fillRect( op.rect, Qt::color0);
			painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
			break;
		case SCROLLRECT:
			painter.end();

			QRegion exposed;
			canvas.scroll(op.pos.x(), op.pos.y(),
			 	op.rect, &exposed);

			painter.begin(&canvas);
			painter.fillRect(exposed.boundingRect(), op.color);
			break;
		}
	}
}


void QVimShell::paintEvent ( QPaintEvent *ev )
{
	flushPaintOps();
	QPainter realpainter(this);
	realpainter.drawPixmap( ev->rect(), canvas, ev->rect());
}

//
// Mouse events
// FIXME: not handling modifiers

void QVimShell::mouseMoveEvent(QMouseEvent *ev)
{	
	if ( ev->buttons() ) {
		gui_send_mouse_event(MOUSE_DRAG, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	} else {
		gui_mouse_moved(ev->pos().x(), ev->pos().y());
	}
	m_input = true;
}

void QVimShell::mousePressEvent(QMouseEvent *ev)
{
	int but;
	setMouseTracking(true);

	switch( ev->button() ) {
	case Qt::LeftButton:
		but = MOUSE_LEFT;
		break;
	case Qt::RightButton:
		but = MOUSE_RIGHT;
		break;
	case Qt::MiddleButton:
		but = MOUSE_MIDDLE;
		break;
	default:
		return;
	}

	int repeat=0;

	if ( !m_lastClick.isNull() 
		&& m_lastClick.elapsed() < QApplication::doubleClickInterval() 
		&& m_lastClickEvent == ev->button() ) {
		repeat = 1;
	}

	m_lastClick.restart();
	m_lastClickEvent = ev->button();
	gui_send_mouse_event(but, ev->pos().x(),
					  ev->pos().y(), repeat, 0);
	m_input = true;
}

void QVimShell::mouseReleaseEvent(QMouseEvent *ev)
{
	gui_send_mouse_event(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	m_input = true;
	setMouseTracking(false);
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	gui_send_mouse_event((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
					    ev->pos().x(), ev->pos().y(), FALSE, 0);
	m_input = true;
}

QIcon QVimShell::iconFromTheme(const QString& name)
{
	QIcon icon;

	// Theme icons
	if ( "Open" == name ) {
		icon = QIcon::fromTheme("document-open", 
				QIcon(":/icons/document-open.png"));
	} else if ( "Save" == name ) {
		icon = QIcon::fromTheme("document-save",
				QIcon(":/icons/document-save.png"));
	} else if ( "SaveAll" == name ) {
		icon = QIcon::fromTheme("document-save-all",
				QIcon(":/icons/document-save-all.png"));
	} else if ( "Print" == name ) {
		icon = QIcon::fromTheme("document-print",
				QIcon(":/icons/document-print.png"));
	} else if ( "Undo" == name ) {
		icon = QIcon::fromTheme("edit-undo",
				QIcon(":/icons/edit-undo.png"));
	} else if ( "Redo"== name ) {
		icon = QIcon::fromTheme("edit-redo",
				QIcon(":/icons/edit-redo.png"));
	} else if ( "Cut" == name ) {
		icon = QIcon::fromTheme("edit-cut",
				QIcon(":/icons/edit-cut.png"));
	} else if ( "Copy" == name ) {
		icon = QIcon::fromTheme("edit-copy",
				QIcon(":/icons/edit-copy.png"));
	} else if ( "Paste" == name ) {
		icon = QIcon::fromTheme("edit-paste",
				QIcon(":/icons/edit-paste.png"));
	} else if ( "Replace" == name ) {
		icon = QIcon::fromTheme("edit-find-replace",
				QIcon(":/icons/edit-find-replace.png"));
	} else if ( "FindNext" == name ) {
		icon = QIcon::fromTheme("go-next",
				QIcon(":/icons/go-next.png"));
	} else if ( "FindPrev" == name ) {
		icon = QIcon::fromTheme("go-previous",
				QIcon(":/icons/go-previous.png"));
	} else if ( "LoadSesn" == name ) {
		icon = QIcon::fromTheme("folder-open",
				QIcon(":/icons/document-open-folder.png"));
	} else if ( "SaveSesn" == name ) {
		icon = QIcon::fromTheme("document-save-as",
				QIcon(":/icons/document-save-as.png"));
	} else if ( "RunScript" == name ) {
		icon = QIcon::fromTheme("system-run",
				QIcon(":/icons/system-run.png"));
	} else if ( "Make" == name ) {
		icon = QIcon::fromTheme("run-build", 
				QIcon(":/icons/run-build.png"));
	} else if ( "RunCtags" == name ) {
		icon = QIcon(":/icons/table.png");
	} else if ( "TagJump" == name ) {
		icon = QIcon::fromTheme("go-jump",
				QIcon(":/icons/go-jump.png"));
	} else if ( "Help" == name ) {
		icon = QIcon::fromTheme("help-contents",
				QIcon(":/icons/help-contents.png"));
	} else if ( "FindHelp" == name ) {
		icon = QIcon::fromTheme("help-faq",
				QIcon(":/icons/help-contextual.png"));
	}

	if ( icon.isNull() ) { // last resort
		icon = QIcon::fromTheme(name.toLower());
	}

	return icon;
}


/**
 * Get an icon for a given name
 *
 */
QIcon QVimShell::icon(const QString& name)
{
	QIcon icon = iconFromTheme(name);

	if ( icon.isNull() ) {
		return QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon);
	}

	return icon;
}

void QVimShell::loadColors(const QString& name)
{
	QFile f(name);
	if (!f.open(QFile::ReadOnly)) {
		return;
	}
	
	while (!f.atEnd()) {
		QString line = convertFrom( f.readLine() );
		if ( line.startsWith("!") ) {
			continue;
		}

		// FIXME: some color names have spaces
		QStringList list = line.split( " ", QString::SkipEmptyParts);
		if ( list.size() != 4 ) {
			continue;
		}

		int r,g,b;
		bool ok_r, ok_g, ok_b;

		r = list[0].toUInt(&ok_r);
		g = list[1].toUInt(&ok_g);
		b = list[2].toUInt(&ok_b);
		if ( !ok_r || !ok_g || !ok_b ) {
			continue;
		}

		QColor c(r,g,b);
		m_colorTable.insert(list[3].simplified(), c);
	}
}

QColor QVimShell::color(const QString& name)
{

    static const char *const vimnames[][2] =
    {
	{"LightRed",	"#FFBBBB"},
	{"LightGreen",	"#88FF88"},
	{"LightMagenta","#FFBBFF"},
	{"DarkCyan",	"#008888"},
	{"DarkBlue",	"#0000BB"},
	{"DarkRed",	"#BB0000"},
	{"DarkMagenta", "#BB00BB"},
	{"DarkGrey",	"#BBBBBB"},
	{"DarkYellow",	"#BBBB00"},
	{"Gray10",	"#1A1A1A"},
	{"Grey10",	"#1A1A1A"},
	{"Gray20",	"#333333"},
	{"Grey20",	"#333333"},
	{"Gray30",	"#4D4D4D"},
	{"Grey30",	"#4D4D4D"},
	{"Gray40",	"#666666"},
	{"Grey40",	"#666666"},
	{"Gray50",	"#7F7F7F"},
	{"Grey50",	"#7F7F7F"},
	{"Gray60",	"#999999"},
	{"Grey60",	"#999999"},
	{"Gray70",	"#B3B3B3"},
	{"Grey70",	"#B3B3B3"},
	{"Gray80",	"#CCCCCC"},
	{"Grey80",	"#CCCCCC"},
	{"Gray90",	"#E5E5E5"},
	{"Grey90",	"#E5E5E5"},
	{"grey15",	"#262626"},
	{"grey20",	"#333333"},
	{"grey40",	"#666666"},
	{"grey50",	"#7f7f7f"},
	{"lightred",	"#FFA0A0"},
	{"yellow2",	"#EEEE00"},
	{"lightmagenta",	"#f0a0f0"},

	{NULL, NULL}
    };

	if ( QColor::isValidColor(name) ) {
		return QColor(name);
	}

	int i;
	for ( i=0; vimnames[i][0] != NULL; i++ ) {

		if ( name == vimnames[i][0] ) {
			return QColor(vimnames[i][1]);
		}
	}

	return QVimShell::m_colorTable.value( name, QColor());
}

void QVimShell::queuePaintOp(PaintOperation op)
{
	paintOps.enqueue(op);
	if ( op.rect.isValid() ) {
		update(op.rect);
	} else {
		update();
	}
}

void QVimShell::setEncodingUtf8(bool enabled)
{
	m_encoding_utf8 = enabled;
}


