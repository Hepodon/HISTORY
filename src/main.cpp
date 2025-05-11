#include "main.h"
#include "liblvgl/core/lv_obj.h"
#include "liblvgl/display/lv_display.h"
#include "liblvgl/misc/lv_area.h"
#include "liblvgl/widgets/label/lv_label.h"
#include <algorithm>
#include <vector>

#define GRAVITY 0.5
#define JUMP_FORCE -5
#define MAX_JUMP_TIME 15
#define MOVE_SPEED 3
#define SCREEN_WIDTH 260
#define SCREEN_HEIGHT 240

using namespace pros;

const char *random_messages[] = {
    "Tito was born in 1892 in a small village in whats now Croatia. Back "
    "then, it was part of the AustroHungarian Empire. He didnt come from "
    "power he was the seventh of fifteen kids and worked as a metalworker "
    "before getting into politics. His early life was shaped by war, poverty, "
    "and a drive to change things, which set the stage for his future as a "
    "revolutionary leader.",
    "During World War II, Tito led a resistance group called the Partisans. "
    "They fought against the Nazis and local fascist forces, becoming one of "
    "the strongest underground movements in Europe. Unlike other resistance "
    "fighters, Titos group included people from all over Yugoslavia Serbs, "
    "Croats, Bosnians, and more. They managed to liberate parts of the country "
    "even before the Allies arrived.",
    "After WWII, Tito became the leader of Yugoslavia and stayed in power "
    "for over three decades. He kept the country together despite all its "
    "different cultures and languages. His style of socialism was unique he "
    "didnt follow Stalin and even kicked Soviet advisors out in 1948. That "
    "bold move made him a symbol of independence during the Cold War.",
    "Tito faced a lot of problems running such a diverse country. Ethnic "
    "groups didnt always get along, and he had to be tough to keep peace. He "
    "banned nationalist parties and used secret police to quiet opposition. It "
    "wasnt exactly democratic, but it kept Yugoslavia from falling apart at "
    "least while he was alive.",
    "Tito loved luxury. He wore custommade suits, "
    "had his own personal train, and even kept a pet tiger for a while. World "
    "leaders from both the East and West visited him, and he threw some pretty "
    "lavish parties. Despite being a communist, Tito definitely enjoyed the "
    "finer things in life."};

struct Platform {
  int x, y, width;
  lv_obj_t *obj;
  bool solid;
};

lv_obj_t *player;
float playerX = 100, playerY = 120;
float velocityY = 0;
bool onGround = false;
int jumpTimer = 0;
bool isJumping = false;
std::vector<Platform> platforms;

Controller master(E_CONTROLLER_MASTER);

void create_platform(int x, int y, bool solid = false, bool ground = false) {
  Platform p;
  p.x = x;
  p.y = y;
  p.width = ground ? SCREEN_WIDTH : 60;
  p.solid = solid;
  p.obj = lv_obj_create(lv_screen_active());
  lv_obj_set_size(p.obj, p.width, 10);
  lv_color_t color =
      solid ? lv_color_hex(0x654321) : lv_color_hex(rand() % 0xFFFFFF);
  lv_obj_set_style_bg_color(p.obj, color, LV_PART_MAIN);
  lv_obj_set_pos(p.obj, p.x, p.y);
  platforms.push_back(p);
}

void create_player() {
  player = lv_obj_create(lv_screen_active());
  lv_obj_set_size(player, 20, 20);
  lv_obj_set_style_bg_color(player, lv_color_hex(0x0000FF), LV_PART_MAIN);
  lv_obj_set_pos(player, playerX, playerY);
}

void show_random_textbox() {
  const char *text = random_messages[rand() % 5];

  // Create a label object
  lv_obj_t *label = lv_label_create(lv_screen_active());

  // Set the text and enable auto-wrapping
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

  // Resize and center it on screen
  lv_obj_set_width(label, 480 - SCREEN_WIDTH); // Fits within Brain screen (480px wide)
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, SCREEN_WIDTH, 0);

  // Optionally wait before clearing
  pros::delay(10000);
  lv_obj_delete(label);
}

void game_loop() {
  while (true) {
    lv_timer_handler();
    float lastY = playerY;

    // Controller-based movement
    int xInput = master.get_analog(ANALOG_LEFT_X);
    if (xInput < -10)
      playerX -= MOVE_SPEED;
    else if (xInput > 10)
      playerX += MOVE_SPEED;

    if (playerX < 0)
      playerX = 0;
    if (playerX > SCREEN_WIDTH - 20)
      playerX = SCREEN_WIDTH - 20;

    // Jumping
    bool jumpHeld = master.get_digital(DIGITAL_A);
    if (onGround && jumpHeld && !isJumping) {
      velocityY = JUMP_FORCE;
      isJumping = true;
      jumpTimer = 0;
      onGround = false;
    }

    if (isJumping && jumpHeld && jumpTimer < MAX_JUMP_TIME) {
      velocityY = JUMP_FORCE;
      jumpTimer++;
    } else {
      isJumping = false;
    }

    velocityY += GRAVITY;
    playerY += velocityY;

    // Collision
    onGround = false;
    for (auto &p : platforms) {
      bool falling = velocityY >= 0;
      bool wasAbove = lastY + 20 <= p.y;
      bool isLanding = playerY + 20 >= p.y && playerY + 20 <= p.y + 10;
      bool aligned = playerX + 20 >= p.x && playerX <= p.x + p.width;

      if (falling && wasAbove && isLanding && aligned) {
        playerY = p.y - 20;
        velocityY = 0;
        onGround = true;
        isJumping = false;
      }
    }

    // Scrolling
    if (playerY < 80) {
      float delta = 80 - playerY;
      playerY = 80;
      for (auto &p : platforms) {
        if (!p.solid) {
          p.y += delta;
          lv_obj_set_y(p.obj, p.y);
        }
      }
    }

    // Cleanup
    platforms.erase(std::remove_if(platforms.begin(), platforms.end(),
                                   [](Platform &p) {
                                     if (!p.solid && p.y > SCREEN_HEIGHT) {
                                       lv_obj_delete(p.obj);
                                       return true;
                                     }
                                     return false;
                                   }),
                    platforms.end());

    // Spawn new platforms
    while (platforms.size() < 7) {
      int x = rand() % (SCREEN_WIDTH - 60);
      int y = platforms.empty() ? 200 : platforms.back().y - (30 + rand() % 20);
      create_platform(x, y);
    }

    lv_obj_set_pos(player, playerX, playerY);
    pros::delay(20);
  }
}

void initialize() {
  lv_init();

  pros::Task([]() {
    while (true) {
      static uint32_t last = pros::millis();
      uint32_t now = pros::millis();
      lv_tick_inc(now - last);
      last = now;
      pros::delay(1);
    }
  });

  create_player();
  create_platform(0, SCREEN_HEIGHT - 10, true, true);

  for (int i = 0; i < 6; ++i) {
    int x = rand() % (SCREEN_WIDTH - 60);
    int y = 200 - i * 40;
    create_platform(x, y);
  }

  // Random textbox task
  pros::Task([]() {
    while (true) {
      pros::delay(5000 + rand() % 3000);
      show_random_textbox();
    }
  });
  (pros::Task(game_loop));
}
