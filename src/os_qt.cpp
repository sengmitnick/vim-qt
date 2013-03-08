

#include <QDir>
#include <QHostInfo>
#include <QApplication>
#include <QDebug>
#include "qt/vimwrapper.h"

// This should be the last one
#include "vim.h"

// TODO: permission checking in NTFS

#ifndef UNIX
char_u e_screenmode[] = "E359: Screen mode setting not supported";
#endif


extern "C" {

void mch_settitle(char_u *title, char_u *icon)
{
	gui_mch_settitle(title, icon);
}

void mch_restore_title(int t)
{
	qDebug() << __func__ << t;
	// WHAT?
}

int mch_chdir(char *path)
{
	return QDir::setCurrent(path) ? TRUE : FALSE;
}

/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory or upon error
 */
int mch_isdir(char_u *name)
{
	return QFileInfo((char*)name).isDir() ? TRUE : FALSE;
}

/*
 * Return TRUE if "fname" does not depend on the current directory.
 */
int mch_isFullName(char_u *fname)
{
	return QFileInfo((char*)fname).isAbsolute() ? TRUE: FALSE;
}

/*
 * Insert host name in s[len].
 */
void mch_get_host_name(char_u *s, int len)
{
	QString name = QHostInfo::localHostName().mid(0, len);
	vim_strncpy(s, (char_u *)VimWrapper::convertTo(name).constData(), len - 1);
}

/*
 * return process ID
 */
long mch_get_pid(void)
{
	return QCoreApplication::applicationPid();
}

int mch_can_exe(char_u *name)
{

	return -1;
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
int mch_dirname(char_u *buf, int len)
{
	QByteArray p = VimWrapper::convertTo(QDir::currentPath());
	if ( p.size() > len) {
		return FAIL;
	}
	vim_strncpy(buf, (char_u *)p.constData(), len - 1);
	return OK;
}

/*
 * set screen mode, always fails.
 */
int mch_screenmode(char_u *arg)
{
	EMSG(_(e_screenmode));
	return FAIL;
}

int mch_rename(const char *oldFile, const char *newFile)
{
	// Return -1 on failuer, if file exists
	return QFile(oldFile).rename(newFile) ? 0 : -1;
}

/*
 * Shut down and exit with status `r'
 * Careful: mch_exit() may be called before mch_init()!
 */
void mch_exit(int r)
{
	qDebug() << __func__ << r;
	display_errors();
	ml_close_all(TRUE);		/* remove all memfiles */

# ifdef FEAT_NETBEANS_INTG
	if (WSInitialized)
	{
		WSInitialized = FALSE;
		WSACleanup();
	}
# endif

	if (gui.in_use)
		gui_exit(r);

#ifdef EXITFREE
	free_all_mem();
#endif

	exit(r);
}

/*
 * Place absolute path to fname in buf
 */
int mch_FullName( char_u *fname, char_u *buf, int len, int force)
{
	// force is ignore (as TRUE)
	//
	QByteArray p = VimWrapper::convertTo(QFileInfo((char*)fname).absoluteFilePath());
	
	if ( p.isEmpty() ||  p.size() > len -1 ) {
		return FAIL;
	}
	vim_strncpy(buf, (char_u *)p.constData(), len - 1);
	return OK;
}

int mch_check_win( int argc, char **argv)
{
	return FAIL;
}

void mch_init()
{
	Columns = 80;
	Rows = 24;

	out_flush();
	//set_signals();
}


void mch_early_init(void)
{


}

/*
 * Return TRUE if the input comes from a terminal, FALSE otherwise.
 */
int mch_input_isatty()
{
	return FALSE;
}

int vim_is_xterm(char_u *)
{
	return FALSE;
}

int mch_can_restore_title()
{
    return TRUE;
}

int mch_can_restore_icon()
{
    return FALSE;
}

void mch_hide(char_u *)
{
    /* can't hide a file */
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
long mch_getperm(char_u *name)
{
	QFileInfo f((char*)name);

	if (!f.exists()) {
		return -1;
	}

	QFile::Permissions perm = f.permissions();
	long p = 0;

	p |= (perm & QFile::ReadOwner) ? 0400 : 0;
	p |= (perm & QFile::WriteOwner) ? 0200 : 0;
	p |= (perm & QFile::ExeOwner) ? 0100 : 0;

	p |= (perm & QFile::ReadGroup) ? 0040 : 0;
	p |= (perm & QFile::WriteGroup) ? 0020 : 0;
	p |= (perm & QFile::ExeGroup) ? 0010 : 0;

	p |= (perm & QFile::ReadOther) ? 0004 : 0;
	p |= (perm & QFile::WriteOther) ? 0002 : 0;
	p |= (perm & QFile::ExeOther) ? 0001 : 0;

	return p;
}

/*
 * set file permission for 'name' to 'perm'
 *
 * return FAIL for failure, OK otherwise
 */
int mch_setperm(char_u *name, long perm)
{
	QFile f((char*)name);
	QFile::Permissions newperm;

	newperm |= (perm & 0400) ? QFile::ReadOwner : QFile::Permissions();
	newperm |= (perm & 0200) ? QFile::WriteOwner : QFile::Permissions();
	newperm |= (perm & 0100) ? QFile::ExeOwner : QFile::Permissions();

	newperm |= (perm & 0040) ? QFile::ReadGroup : QFile::Permissions();
	newperm |= (perm & 0020) ? QFile::WriteGroup : QFile::Permissions();
	newperm |= (perm & 0010) ? QFile::ExeGroup : QFile::Permissions();

	newperm |= (perm & 0004) ? QFile::ReadOther : QFile::Permissions();
	newperm |= (perm & 0002) ? QFile::WriteOther : QFile::Permissions();
	newperm |= (perm & 0001) ? QFile::ExeOther : QFile::Permissions();

	return f.setPermissions(newperm) ? TRUE : FALSE;
}


void mch_delay( long msec, int ignoreinput)
{

}

vim_acl_T mch_get_acl(char_u *fname)
{
	return NULL;
}

void mch_set_acl(char_u *fname, vim_acl_T acl)
{
}


void mch_free_acl(vim_acl_T acl)
{
}

/*
 * Check what "name" is:
 * NODE_NORMAL: file or directory (or doesn't exist)
 * NODE_WRITABLE: writable device, socket, fifo, etc.
 * NODE_OTHER: non-writable things
 */
int mch_nodetype(char_u *name)
{
	qDebug() << __func__ << (char*) name;

	QFileInfo fi((char*)name);

	if ( fi.isFile() ) {
		QFile f((char*)name);
		if ( f.isSequential() ) {
			return NODE_WRITABLE;
		} else {
			return NODE_NORMAL;
		}
	} else {
		return NODE_NORMAL;
	}

	return NODE_OTHER;
}

void mch_breakcheck(void)
{
	// FIXME:
	//qDebug() << __func__;
}


void mch_settmode(int tmode)
{
	/* nothing to do */
}


int mch_get_shellsize()
{
	qDebug() << __func__;
	return FAIL;
}

/*
 * Insert user name in s[len].
 */
int mch_get_user_name(char_u *s, int len)
{
	char *username = getenv("USER");
	if ( username == NULL ) {
		username = getenv("USERNAME");
	}

	if ( username != NULL) {
		vim_strncpy(s, (char_u *)username, len - 1);
		return OK;
	}
	s[0] = 0;
	return FAIL;
}

/*
 * Either execute a command by calling the shell or start a new shell
 */
int mch_call_shell(char_u *cmd, int options)	/* SHELL_*, see vim.h */
{
	qDebug() << __func__ << (char*)cmd;
	return FAIL;
}

/*
 * Return TRUE if the string "p" contains a wildcard.
 * Don't recognize '~' at the end as a wildcard.
 */
int mch_has_wildcard(char_u *p)
{
	for ( ; *p; mb_ptr_adv(p)) {
#ifndef OS2
		if (*p == '\\' && p[1] != NUL) {
			++p;
		} else {
#endif
			if (vim_strchr((char_u *)
#ifdef VMS
						"*?%$"
#else
# ifdef OS2
#  ifdef VIM_BACKTICK
						"*?$`"
#  else
						"*?$"
#  endif
# else
						"*?[{`'$"
# endif
#endif
						, *p) != NULL
					|| (*p == '~' && p[1] != NUL)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*
 * mch_expand_wildcards() - this code does wild-card pattern matching using
 * the shell
 *
 * return OK for success, FAIL for error (you may lose some memory) and put
 * an error message in *file.
 *
 * num_pat is number of input patterns
 * pat is array of pointers to input patterns
 * num_file is pointer to number of matched file names
 * file is pointer to array of pointers to matched file names
 */
int mch_expand_wildcards(int num_pat, char_u **pat, int *num_file, char_u ***file, int flags)
{
	qDebug() << __func__;
	for ( int i=0; i<num_pat; i++ ) {
		qDebug() << "\t" << (char*)pat[i];
	}

	*num_file = 0;

	return FAIL;
}

/*
 * Return TRUE if the string "p" contains a wildcard that mch_expandpath() can
 * expand.
 */
int mch_has_exp_wildcard(char_u *p)
{
	for ( ; *p; mb_ptr_adv(p)) {
#ifndef OS2
		if (*p == '\\' && p[1] != NUL) {
			++p;
		} else {
#endif
			if (vim_strchr((char_u *)
#ifdef VMS
						"*?%"
#else
# ifdef OS2
						"*?"
# else
						"*?[{'"
# endif
#endif
						, *p) != NULL) {

				return TRUE;
			}
		}
	}
	return FALSE;
}

/*
 * Expand a path into all matching files and/or directories.  Handles "*",
 * "?", "[a-z]", "**", etc.
 * "path" has backslashes before chars that are not to be expanded.
 * Returns the number of matches found.
 */
int mch_expandpath(garray_T *gap, char_u *path, int flags)
{
	int start_len = gap->ga_len;
	QRegExp r = QRegExp((char*)path);
	r.setPatternSyntax(QRegExp::WildcardUnix);
	r.setCaseSensitivity(flags & EW_ICASE ? Qt::CaseInsensitive : Qt::CaseSensitive);

	QString p((char*)path);
	int i, pos=-1;
	if ( (i=p.indexOf('*')) != -1 ) {
		pos = i;
	}
	if ( (i=p.indexOf('?')) != -1 && i<pos ) {
		pos = i;
	}
	if ( (i=p.indexOf('[')) != -1 && i<pos ) {
		pos = i;
	}

	QFileInfo base;
	if ( pos == -1 ) {
		// No pattern?
		return 0;
	} else if ( pos == 0 ) {
		// Base is the current dir
		base.setFile(".");
	} else {
		QFileInfo fi(p.mid(0, pos));
		if ( fi.isDir() ) {
			base = fi;
		} else {
			base = QFileInfo(fi.path());
		}
	}

	//qDebug() << __func__ << (char*) path << base.filePath();
	QFileInfoList pending, matches;
	pending << base;
	while ( pending.size()) {

		QFileInfo fi = pending.takeFirst();

		QDir::Filters filter = QDir::NoDotAndDotDot | QDir::AllEntries;
		QDir dir(fi.filePath());
		foreach ( QFileInfo entry, dir.entryInfoList(filter)) {
			if ( r.exactMatch(entry.filePath()) ) {
				matches.append(entry);
			}

			if ( entry.isDir() && r.matchedLength() != 0 ) {
				pending.append(entry);
			}
		}
	}
	
	foreach(QFileInfo entry, matches) {
		addfile(gap, (char_u*)VimWrapper::convertTo(entry.filePath()).constData(), flags);
	}

	return gap->ga_len - start_len;
}


/*
 * We have no job control, so fake it by starting a new shell.
 */
void mch_suspend()
{
	qDebug() << __func__;
	//suspend_shell();
}

void mch_setmouse(int)
{
	qDebug() << __func__;
}

void mch_set_shellsize()
{
	qDebug() << __func__;
}

void mch_new_shellsize()
{
    /* Nothing to do. */
}


} // extern "C"

