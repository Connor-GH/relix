// Shell.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>

// Parsed command representation
#define EXEC 1
#define REDIR 2
#define PIPE 3
#define LIST 4
#define BACK 5
extern char **environ;

void
sigint_handler(int signum)
{
}

struct cmd {
	int type;
};

struct execcmd {
	int type;
	char *argv[MAXARG];
	char *eargv[MAXARG];
};

struct redircmd {
	int type;
	struct cmd *cmd;
	char *file;
	char *efile;
	int flags;
	int fd;
};

struct pipecmd {
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct listcmd {
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct backcmd {
	int type;
	struct cmd *cmd;
};

struct cmd *execcmd(void);

int fork1(void); // Fork but panics on failure.
void __attribute__((noreturn)) panic(char *);
struct cmd *parsecmd(char *);

static char pwd[__DIRSIZ] = "/";
// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
	char *str;
	int p[2];
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == NULL) {
		exit(1);
	}
	switch (cmd->type) {
	default:
		panic("runcmd");

	case EXEC:
		str = malloc(__DIRSIZ);
		ecmd = (struct execcmd *)cmd;
		if (ecmd->argv[0] == NULL) {
			exit(1);
		}

		execvp(ecmd->argv[0], ecmd->argv);
		fprintf(stderr, "exec %s failed: %s\n", ecmd->argv[0], strerror(errno));
		break;

	case REDIR:
		rcmd = (struct redircmd *)cmd;
		close(rcmd->fd);
		int rcmd_fd;
		if ((rcmd_fd = open(rcmd->file, rcmd->flags, 0777)) < 0) {
			fprintf(stderr, "open %s failed\n", rcmd->file);
			perror("open");
			exit(1);
		}
		runcmd(rcmd->cmd);
		close(rcmd_fd);
		break;

	case LIST:
		lcmd = (struct listcmd *)cmd;
		if (fork1() == 0) {
			runcmd(lcmd->left);
		}
		wait(NULL);
		runcmd(lcmd->right);
		break;

	case PIPE:
		pcmd = (struct pipecmd *)cmd;
		if (pipe(p) < 0) {
			panic("pipe");
		}
		if (fork1() == 0) {
			close(1);
			assert(dup(p[1]) != -1);
			close(p[0]);
			close(p[1]);
			runcmd(pcmd->left);
		}
		if (fork1() == 0) {
			close(0);
			assert(dup(p[0]) != -1);
			close(p[0]);
			close(p[1]);
			runcmd(pcmd->right);
		}
		close(p[0]);
		close(p[1]);
		wait(NULL);
		wait(NULL);
		break;

	case BACK:
		bcmd = (struct backcmd *)cmd;
		if (fork1() == 0) {
			runcmd(bcmd->cmd);
		}
		break;
	}
	exit(0);
}

int
getcmd(char *buf, int nbuf)
{
	fprintf(stdout, "$ ");
	memset(buf, 0, nbuf);
	if (fgets(buf, nbuf, stdin) != buf) {
		perror("fgets");
		exit(1);
	}
	if (buf[0] == 0) { // EOF
		return -1;
	}
	return 0;
}

int
main(void)
{
	static char buf[100];
	int fd;

	// Ensure that three file descriptors are open.
	while ((fd = open("console", O_RDWR)) >= 0) {
		if (fd >= 3) {
			close(fd);
			break;
		}
	}
	sighandler_t sighandler = signal(SIGINT, sigint_handler);
	if (sighandler == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	// TODO make more general builtin parser.
	// Read and run input commands.
	while (getcmd(buf, sizeof(buf)) >= 0) {
		if (buf[0] == 'c' && buf[1] == 'd' && isspace(buf[2])) {
			// Chdir must be called by the parent, not the child.
			buf[strlen(buf) - 1] = 0; // chop \n
			if (strlen(buf) >= 3) {
				if (chdir(buf + 3) < 0) {
					fprintf(stderr, "cannot cd %s: %s\n", buf + 3, strerror(errno));
				} else {
					strcpy(pwd, buf);
				}
				continue;
			} else {
				char *home = getenv("HOME");
				if (home == NULL) {
					fprintf(stderr, "$HOME is not set. It is needed for `cd'"
					                " without arguments.\n");
					exit(1);
				}
				// `cd' with no arguments.`
				if (chdir(home) < 0) {
					fprintf(stderr, "cannot cd %s: %s\n", buf + 3, strerror(errno));
				} else {
					strcpy(pwd, buf);
				}
				continue;
			}
		}
		int status;
		int pid = fork1();
		if (pid == 0) {
			runcmd(parsecmd(buf));
		}
		pid = wait(&status);
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "pid %d: %s (%d)\n", pid, strsignal(WTERMSIG(status)),
			        WEXITSTATUS(status));
		} else if (WEXITSTATUS(status) != 0 && buf[0] != '\n') {
			fprintf(stderr, "ERROR: pid %d returned with status %d\n", pid,
			        WEXITSTATUS(status));
		}
	}
	return 0;
}

void __attribute__((noreturn))
panic(char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(1);
}

int
fork1(void)
{
	int pid;

	pid = fork();
	if (pid == -1) {
		panic("fork");
	}
	return pid;
}

// Constructors

struct cmd *
execcmd(void)
{
	struct execcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = EXEC;
	return (struct cmd *)cmd;
}

struct cmd *
redircmd(struct cmd *subcmd, char *file, char *efile, int flags, int fd)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = REDIR;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->efile = efile;
	cmd->flags = flags;
	cmd->fd = fd;
	return (struct cmd *)cmd;
}

struct cmd *
pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = PIPE;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd *)cmd;
}

struct cmd *
listcmd(struct cmd *left, struct cmd *right)
{
	struct listcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = LIST;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd *)cmd;
}

struct cmd *
backcmd(struct cmd *subcmd)
{
	struct backcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = BACK;
	cmd->cmd = subcmd;
	return (struct cmd *)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
	char *s;
	int ret;

	s = *ps;
	while (s < es && strchr(whitespace, *s)) {
		s++;
	}
	if (q) {
		*q = s;
	}
	ret = *s;
	switch (*s) {
	case 0:
		break;
	case '|':
	case '(':
	case ')':
	case ';':
	case '&':
	case '<':
		s++;
		break;
	case '>':
		s++;
		if (*s == '>') {
			ret = '+';
			s++;
		}
		break;
	default:
		ret = 'a';
		while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)) {
			s++;
		}
		break;
	}
	if (eq) {
		*eq = s;
	}

	while (s < es && strchr(whitespace, *s)) {
		s++;
	}
	*ps = s;
	return ret;
}

int
peek(char **ps, char *es, char *toks)
{
	char *s;

	s = *ps;
	while (s < es && strchr(whitespace, *s)) {
		s++;
	}
	*ps = s;
	return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

struct cmd *
parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parseline(&s, es);
	peek(&s, es, "");
	if (s != es) {
		fprintf(stderr, "leftovers: %s\n", s);
		panic("syntax error");
	}
	nulterminate(cmd);
	return cmd;
}

struct cmd *
parseline(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parsepipe(ps, es);
	while (peek(ps, es, "&")) {
		gettoken(ps, es, NULL, NULL);
		cmd = backcmd(cmd);
	}
	if (peek(ps, es, ";")) {
		gettoken(ps, es, NULL, NULL);
		cmd = listcmd(cmd, parseline(ps, es));
	}
	return cmd;
}

struct cmd *
parsepipe(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parseexec(ps, es);
	if (peek(ps, es, "|")) {
		gettoken(ps, es, NULL, NULL);
		cmd = pipecmd(cmd, parsepipe(ps, es));
	}
	return cmd;
}

struct cmd *
parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	char *q, *eq;

	while (peek(ps, es, "<>")) {
		tok = gettoken(ps, es, NULL, NULL);
		if (gettoken(ps, es, &q, &eq) != 'a') {
			panic("missing file for redirection");
		}
		switch (tok) {
		case '<':
			cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
			break;
		case '>':
			cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
			break;
		case '+': // >>
			cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
			break;
		}
	}
	return cmd;
}

struct cmd *
parseblock(char **ps, char *es)
{
	struct cmd *cmd;

	if (!peek(ps, es, "(")) {
		panic("parseblock");
	}
	gettoken(ps, es, NULL, NULL);
	cmd = parseline(ps, es);
	if (!peek(ps, es, ")")) {
		panic("syntax - missing )");
	}
	gettoken(ps, es, NULL, NULL);
	cmd = parseredirs(cmd, ps, es);
	return cmd;
}

struct cmd *
parseexec(char **ps, char *es)
{
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd;
	struct cmd *ret;

	if (peek(ps, es, "(")) {
		return parseblock(ps, es);
	}

	ret = execcmd();
	cmd = (struct execcmd *)ret;

	argc = 0;
	ret = parseredirs(ret, ps, es);
	while (!peek(ps, es, "|)&;")) {
		if ((tok = gettoken(ps, es, &q, &eq)) == 0) {
			break;
		}
		if (tok != 'a') {
			panic("syntax error: expected `a'");
		}
		cmd->argv[argc] = q;
		cmd->eargv[argc] = eq;
		argc++;
		if (argc >= MAXARG) {
			panic("too many args");
		}
		ret = parseredirs(ret, ps, es);
	}
	cmd->argv[argc] = NULL;
	cmd->eargv[argc] = NULL;
	return ret;
}

// NUL-terminate all the counted strings.
struct cmd *
nulterminate(struct cmd *cmd)
{
	int i;
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == NULL) {
		return NULL;
	}

	switch (cmd->type) {
	case EXEC:
		ecmd = (struct execcmd *)cmd;
		for (i = 0; ecmd->argv[i]; i++) {
			*ecmd->eargv[i] = 0;
		}
		break;

	case REDIR:
		rcmd = (struct redircmd *)cmd;
		nulterminate(rcmd->cmd);
		*rcmd->efile = 0;
		break;

	case PIPE:
		pcmd = (struct pipecmd *)cmd;
		nulterminate(pcmd->left);
		nulterminate(pcmd->right);
		break;

	case LIST:
		lcmd = (struct listcmd *)cmd;
		nulterminate(lcmd->left);
		nulterminate(lcmd->right);
		break;

	case BACK:
		bcmd = (struct backcmd *)cmd;
		nulterminate(bcmd->cmd);
		break;
	}
	return cmd;
}
