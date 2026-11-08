#pragma once

#include "bambu_tag.h"

void display_init(void);
void display_show_message(const char *message);
void display_show_tag(const bambu_tag_t *tag);
void display_show_battery(int mv);
