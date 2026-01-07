/*
 * setup.c: Rog-O-Matic XIV (CMU) Wed Jan 30 17:38:07 1985 - mlm
 * Copyright (C) 1985 by A. Appel, G. Jacobson, L. Hamey, and M. Mauldin
 *
 * This is the program which forks and execs the Rogue & the Player
 * 
 * Updated 2024: Use forkpty() instead of pipes for modern ncurses compatibility
 */

# include <stdio.h>
# include <stdlib.h>
# include <signal.h>
# include <unistd.h>
# include <util.h>      /* for forkpty on macOS */
# include <termios.h>
# include "install.h"

/* Forward declarations */
void replaylog(char *fname, char *options);
int author(void);

int   frogue, trogue;

int main (int argc, char *argv[])
{ int   child, score = 0, oldgame = 0;
  int   cheat = 0, noterm = 1, echo = 0, nohalf = 0, replay = 0;
  int   emacs = 0, rf = 0, terse = 0, user = 0, quitat = 2147483647;
  int   debug_trace = 0;    /* Debug tracing flag */
  int   moverate = 0;       /* Moves per second (0 = unlimited) */
  int   mr = 0;             /* Flag for moverate arg */
  char  *rfile = "", *rfilearg = "", options[64];
  char  ropts[128], roguename[128];
  int   master_fd;
  struct termios term;
  struct winsize ws;

  while (--argc > 0 && (*++argv)[0] == '-')
  { while (*++(*argv))
    { switch (**argv)
      { case 'c': cheat++;        break; /* Will use trap arrows! */
        case 'D': debug_trace++;  break; /* Debug tracing to /tmp/rgm.log */
        case 'e': echo++;         break; /* Echo file to roguelog */
        case 'f': rf++;           break; /* Next arg is the rogue file */
        case 'h': nohalf++;       break; /* No halftime show */
        case 'm': mr++;           break; /* Next arg is move rate */
        case 'p': replay++;       break; /* Play back roguelog */
        case 'r': oldgame++;      break; /* Use saved game */
        case 's': score++;        break; /* Give scores only */
        case 't': terse++;        break; /* Give status lines only */
        case 'u': user++;         break; /* Start up in user mode */
        case 'w': noterm = 0;     break; /* Watched mode */
        case 'E': emacs++;        break; /* Emacs mode */
        default:  printf 
                  ("Usage: rogomatic [-cDefhmprstuwE] [-m rate] or rogomatic [file]\n");
                  exit (1);
      }
    }

    if (rf) 
    { if (--argc) rfilearg = *++argv;
      rf = 0;
    }
    if (mr)
    { if (--argc) moverate = atoi(*++argv);
      mr = 0;
    }
  }

  if (argc > 1)
  { printf ("Usage: rogomatic [-cefhprstuwE] or rogomatic <file>\n");
    exit (1);
  }

  /* Find which rogue to use */
  if (*rfilearg)
  { if (access (rfilearg, 1) == 0)	rfile = rfilearg;
    else				{ perror (rfilearg); exit (1); }
  }
  else if (access ("rogue", 1) == 0)	rfile = "rogue";
# ifdef NEWROGUE
  else if (access (NEWROGUE, 1) == 0)	rfile = NEWROGUE;
# endif
# ifdef ROGUE
  else if (access (ROGUE, 1) == 0)	rfile = ROGUE;
# endif
  else
  { perror ("rogue");
    exit (1);
  }

  if (!replay && !score) quitat = findscore (rfile, "Rog-O-Matic");

  sprintf (options, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
           cheat, noterm, echo, nohalf, emacs, terse, user, quitat, debug_trace, moverate);
  sprintf (roguename, "Rog-O-Matic %s for %s", RGMVER, getname ());
  sprintf (ropts, "name=%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
           roguename, "fruit=apricot", "terse", "noflush", "noask",
           "jump", "step", "nopassgo", "inven=slow", "seefloor");

  if (score)  { dumpscore (argc==1 ? argv[0] : DEFVER); exit (0); }
  if (replay) { replaylog (argc==1 ? argv[0] : ROGUELOG, options); exit (0); }

  /* Set up terminal size for the PTY */
  ws.ws_row = 24;
  ws.ws_col = 80;
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  /* Use forkpty to create a pseudo-terminal for rogue */
  child = forkpty(&master_fd, NULL, NULL, &ws);
  
  if (child < 0)
  { perror("forkpty failed");
    exit(1);
  }
  
  if (child == 0)
  { /* Child process - will exec rogue */
    setenv("ROGUEOPTS", ropts, 1);
    setenv("TERM", "rterm", 1);
    setenv("TERMCAP", "rg|rterm|Rog-O-Matic terminal:am:bs:xn:co#80:li#24:ce=\\E^S:cl=^L:cm=\\Ea%+ %+ :so=\\ED:se=\\Ed:pt:ta=^I:up=\\E;:db:", 1);
    
    if (oldgame)  execl (rfile, rfile, "-r", NULL);
    if (argc)     execl (rfile, rfile, argv[0], NULL);
    execl (rfile, rfile, NULL);
    perror("execl rogue failed");
    _exit (1);
  }

  else
  { /* Parent process - will exec player */
    /* Wait briefly for rogue to start and begin output */
    usleep(100000);  /* 100ms startup delay */
    
    /* The master_fd is used for both reading and writing to rogue */
    /* We need two separate fds because player does fdopen on each */
    frogue = master_fd;
    trogue = dup(master_fd);  /* Duplicate for writing */
    
    /* Encode the open files into a two character string */
    char ft[3] = "aa", rp[32]; 
    ft[0] += frogue; 
    ft[1] += trogue;

    /* Pass the process ID of the Rogue process as an ASCII string */
    sprintf (rp, "%d", child);

    if (!author ()) nice (4);

    execl ("./player", "player", ft, rp, options, roguename, NULL);
# ifdef PLAYER
    execl (PLAYER, "player", ft, rp, options, roguename, NULL);
# endif
    perror("execl player failed");
    printf ("Rogomatic not available, 'player' binary missing.\n");
    kill (child, SIGKILL);
  }
  
  return 0;
}

/* 
 * replaylog: Given a log file name and an options string, exec the player
 * process to replay the game.  No Rogue process is needed (since we are
 * replaying an old game), so the frogue and trogue file descrptiors are
 * given the fake value 'Z'.
 */

void replaylog (char *fname, char *options)
{ execl ("./player", "player", "ZZ", "0", options, fname, NULL);
# ifdef PLAYER
  execl (PLAYER, "player", "ZZ", "0", options, fname, NULL);
# endif
  printf ("Replay not available, 'player' binary missing.\n");
  exit (1);
}

/*
 * author:
 *	See if a user is an author of the program
 */

int author(void)
{
  switch (getuid())
  { case 1337:	/* Fuzzy */
    case 1313:	/* Guy */
    case 1241:	/* Andrew */
    case 345:	/* Leonard */
    case 342:	/* Gordon */
		return 1;
    default:	return 0;
  }
}
