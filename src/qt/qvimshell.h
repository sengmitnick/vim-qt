#ifndef __QVIMSHELL__
#define __QVIMSHELL__

#include <QWidget>
#include <QQueue>
#include <QLabel>
#include <QTime>
#include "vimwrapper.h"


class QVimShell: public QWidget, public VimWrapper
{
	Q_OBJECT
	Q_ENUMS(PaintOperationType)
public:
	enum PaintOperationType { CLEARALL=0, FILLRECT, DRAWSTRING, DRAWRECT, INVERTRECT, SCROLLRECT, DRAWSIGN};
	class PaintOperation {
	public:
		enum QVimShell::PaintOperationType type;
		QRect rect;
		QColor color;
		// DRAWSTRING
		QFont font;
		QString str;
		bool undercurl;
		QColor curlcolor;
		// SIGN
		QPixmap sign;
		// SCROLL
		QPoint pos;
	};

	QVimShell(QWidget *parent=0);

	bool hasInput();
	static QColor color(const QString&);

	void queuePaintOp(PaintOperation);

	QColor background();
	int charWidth();

	void setEncodingUtf8(bool);
	virtual QVariant inputMethodQuery(Qt::InputMethodQuery) const;
	void setSlowStringDrawing(bool slow) {m_slowStringDrawing = slow;}

	void setBlinkTime(const long waittime, const long ontime, const long offtime);

	void stopBlinking();
	void startBlinking();


public slots:
	void setBackground(const QColor);
	void setCharWidth(int);

	void close();
	virtual void closeEvent(QCloseEvent *event);

	void switchTab(int idx);
	void closeTab(int idx);

signals:
	void backgroundColorChanged(const QColor&);

protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent ( QKeyEvent *);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event );
	virtual void inputMethodEvent(QInputMethodEvent *event);
	virtual void focusInEvent(QFocusEvent *);
	virtual void focusOutEvent(QFocusEvent *);

	virtual void paintEvent( QPaintEvent *);

	QFont fixPainterFont(const QFont &);
	void drawString(const PaintOperation&, QPainter& );
	void drawStringSlow( const PaintOperation&, QPainter &painter );


	int_u vimKeyboardModifiers(Qt::KeyboardModifiers);
	int_u vimMouseModifiers(Qt::KeyboardModifiers);

	void dragEnterEvent(QDragEnterEvent *);
	void dropEvent(QDropEvent *);

	void tooltip(const QString& );
	void restoreCursor();

	virtual void leaveEvent(QEvent *ev);
	virtual void enterEvent(QEvent *ev);
	bool focusNextPrevChild(bool next);

private slots:
	void cursorOff();
	void cursorOn();
	void startBlinkOffTimer();
	void startBlinkOnTimer();

private:
	QColor m_background;
	int m_charWidth;
	QFont m_font;

	QTimer * timer_cursorBlinkOn ;
	QTimer * timer_cursorBlinkOff ;
	QTimer * timer_firstOff;
	QTimer * timer_firstOn;


	long m_blinkWaitTime, m_blinkOnTime, m_blinkOffTime;

	enum blink_state{BLINK_NONE, BLINK_ON, BLINK_OFF};
	blink_state blinkState;
	bool m_encoding_utf8;

	QQueue<PaintOperation> paintOps;

	QTime m_lastClick;
	int m_lastClickEvent;
	QLabel *m_tooltip;

	bool m_slowStringDrawing;
	bool m_mouseHidden;
};

struct special_key
{
	int key_sym;
	char_u code0;
	char_u code1;
};

static const struct special_key special_keys[] =
{
	{Qt::Key_Up,		'k', 'u'},
	{Qt::Key_Down,		'k', 'd'},
	{Qt::Key_Left,		'k', 'l'},
    	{Qt::Key_Right,		'k', 'r'},

	{Qt::Key_F1,		'k', '1'},
	{Qt::Key_F2,		'k', '2'},
	{Qt::Key_F3,		'k', '3'},
	{Qt::Key_F4,		'k', '4'},
	{Qt::Key_F5,		'k', '5'},
	{Qt::Key_F6,		'k', '6'},
	{Qt::Key_F7,		'k', '7'},
	{Qt::Key_F8,		'k', '8'},
	{Qt::Key_F9,		'k', '9'},
	{Qt::Key_F10,		'k', ';'},
	{Qt::Key_F11,		'F', '1'},
	{Qt::Key_F12,		'F', '2'},
	{Qt::Key_F13,		'F', '3'},
	{Qt::Key_F14,		'F', '4'},
	{Qt::Key_Backspace,	'k', 'b'},

	{Qt::Key_Delete,	'k', 'D'},
	{Qt::Key_Insert,	'k', 'I'},
	{Qt::Key_Home,		'k', 'h'},
	{Qt::Key_End,		'@', '7'},
	{Qt::Key_PageUp,	'k', 'P'},
	{Qt::Key_PageDown,	'k', 'N'},

	{Qt::Key_Print,		'%', '9'},

	// The following are not really **special** but this
	// allows to use these keys with any keyboard modifier while
	// bypassing some of Qt's unwanted behaviour in the text()
	//
	{Qt::Key_Tab,		TAB, NUL},
	{Qt::Key_Backtab,	TAB, NUL},
	{Qt::Key_Escape,	ESC, NUL},
	{Qt::Key_Return,	CAR, NUL},
	{Qt::Key_Enter,		CAR, NUL},
	{Qt::Key_Space,		' ', NUL},

	/* End of list marker: */
	{0, 0, 0}
};

#endif
