#if LAB >= 6

#include <inc/stdio.h>
#include <inc/lib.h>

int debug = 0;

//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug > 1) printf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1) printf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while(strchr(WHITESPACE, *s))
		*s++ = 0;
	if(*s == 0) {
		if (debug > 1) printf("EOL\n");
		return 0;
	}
	if(strchr(SYMBOLS, *s)){
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1) printf("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while(*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;
		**p2 = 0;
		printf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], fd, rightpipe;

	rightpipe = 0;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				printf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if(gettoken(0, &t) != 'w'){
				printf("syntax error: < not followed by word\n");
				exit();
			}
#if SOL >= 6
			if ((fd = open(t, O_RDONLY)) < 0) {
				printf("open %s for read: %e", t, fd);
				exit();
			}
			if(fd != 0){
				dup(fd, 0);
				close(fd);
			}
#else
			// Your code here -- open t for reading,
			// dup it onto fd 0, and then close the fd you got.
			panic("< redirection not implemented");
#endif
			break;
		case '>':
			if(gettoken(0, &t) != 'w'){
				printf("syntax error: > not followed by word\n");
				exit();
			}
#if SOL >= 6
			if ((fd = open(t, O_WRONLY)) < 0) {
				printf("open %s for write: %e", t, fd);
				exit();
			}
			if(fd != 1){
				dup(fd, 1);
				close(fd);
			}
#else
			// Your code here -- open t for writing,
			// dup it onto fd 1, and then close the fd you got.
			panic("> redirection not implemented");
#endif
			break;
		case '|':
#if SOL >= 6
			if((r=pipe(p)) < 0){
				printf("pipe: %e", r);
				exit();
			}
			if (debug) printf("PIPE: %d %d\n", p[0], p[1]);
			if((r=fork()) < 0){
				printf("fork: %e", r);
				exit();
			}
			if(r == 0){
				if(p[0] != 0){
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			}else{
				rightpipe = r;
				if(p[1] != 1){
					dup(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
#else
			// Your code here.
			// 	First, allocate a pipe.
			//	Then fork.
			//	the child runs the right side of the pipe:
			//		dup the read end of the pipe onto 0
			//		close the read end of the pipe
			//		close the write end of the pipe
			//		goto again, to parse the rest of the command line
			//	the parent runs the left side of the pipe:
			//		dup the write end of the pipe onto 1
			//		close the write end of the pipe
			//		close the read end of the pipe
			//		set "rightpipe" to the child envid
			//		goto runit, to execute this piece of the pipeline
			//			and then wait for the right side to finish
#endif
			panic("| not implemented");
			break;
		}
	}

runit:
	if(argc == 0) {
		if (debug) printf("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (debug) {
		printf("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
	}
	if ((r = spawn(argv[0], argv)) < 0)
		printf("spawn %s: %e\n", argv[0], r);
	close_all();
	if (r >= 0) {
		if (debug) printf("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
		wait(r);
		if (debug) printf("[%08x] wait finished\n", env->env_id);
	}
	if (rightpipe) {
		if (debug) printf("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
		if (debug) printf("[%08x] wait finished\n", env->env_id);
	}
	exit();
}

void
usage(void)
{
	printf("usage: sh [-dix] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive, echocmds;

	interactive = '?';
	echocmds = 0;
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1){
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			panic("open %s: %e", r);
		assert(r==0);
	}
	if(interactive == '?')
		interactive = iscons(0);
	for(;;){
		char *buf;

		buf = readline(interactive ? "$ " : NULL);
		if (buf == NULL)
			exit();	// end of file
		if (debug) printf("LINE: %s\n", buf);
		if (buf[0] == '#')
			continue;
		if (echocmds)
			fprintf(1, "# %s\n", buf);
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (r == 0) {
			runcmd(buf);
			exit();
		} else
			wait(r);
	}
}

#endif