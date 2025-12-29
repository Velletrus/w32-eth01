
#include "process_image.h"
void pi_touch() {
  pi_lock();
  PI.tcp_stat[0] = 1;
  pi_unlock();
}
