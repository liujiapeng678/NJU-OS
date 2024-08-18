#include "thread.h"

void Thello(int id) {
  while (1) {
    printf("%c", "_ABCDEFGHIJKLMNOPQRSTUVWXYZ"[id]);
  }
}

int main() {
  for (int i = 0; i < 10; i++) {
    create(Thello);
  }
}
