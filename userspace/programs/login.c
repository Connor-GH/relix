#include "../../kernel/include/proc.h"
#include "../include/user.h"
#include "../include/auth.h"
#include "../../kernel/include/param.h"
#include "../../include/stdlib.h"

int
main(int argc, char **argv)
{
  struct cred *cred;
  char username[MAX_USERNAME];
  char passwd[MAX_PASSWD];
try_again:
  printf("username: ");
  (void)gets(username, MAX_USERNAME);
  username[strlen(username) - 1] = '\0';

  printf("password for %s: ", username);
  echoout(0);
  (void)gets(passwd, MAX_PASSWD);
  passwd[strlen(passwd) - 1] = '\0';
  echoout(1);
  int uid = usertouid(username);
  if (uid == -1) {
    fprintf(stderr, "User does not exist!\n");
    goto try_again;
  }
  char *actual_password;
  usertopasswd(username, actual_password);
  if (actual_password == NULL) {
    exit();
  }
  if (strcmp(passwd, actual_password) == 0) {
    fprintf(stderr, "Password is correct!\n");
    cred->uid = uid;
    cred->gids[0] = uid;
    setuid(uid);
    char *sh_argv[] = {"sh", NULL};
    execv("sh", sh_argv);
    printf("execv sh failed\n");
    exit();
  } else {
    fprintf(stderr, "Password is incorrect.\n");
    fprintf(stderr, "yours: %s theirs: %s\n", passwd, actual_password);
    sleep(100);
    goto try_again;
  }

}
