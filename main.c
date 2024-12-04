#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SW 960.0f
#define SH 640.0f
#define BG (Color){33, 33, 33}

#define BRADIUS 15.0f
#define BSPEED 3.0f
#define PW 15.0f
#define PH 50.0f
#define PSPEED 4.0f
#define AISPEED 2.0f
#define MAXSCORE 5
#define RDELAY 1.0f

typedef struct {
  float rtime;
  float lreact;
  float ty;
  float perror;
  float conf;
  bool active;
  float idle_pos;
} AIState;

typedef struct {
  int p1;
  int p2;
  float rtimer;
  bool waiting;
} Game;

bool ai_left = false;

void Init(Vector2 *bpos, Vector2 *bvel, float *p1pos, float *p2pos,
          bool *ai_enabled, AIState *ai, AIState *ai_left_state, Game *game) {
  *bpos = (Vector2){SW / 2, SH / 2};

  float angle = (rand() % 91) - 45;
  if (rand() % 2 == 0)
    angle += 180;

  *bvel =
      (Vector2){BSPEED * cosf(angle * DEG2RAD), BSPEED * sinf(angle * DEG2RAD)};

  *p1pos = SH / 2 - PH / 2;
  *p2pos = SH / 2 - PH / 2;
  *ai_enabled = false;

  ai->rtime = 0.2f;
  ai->lreact = 0;
  ai->ty = *p2pos;
  ai->perror = 0;
  ai->conf = 0.8f;
  ai->active = false;
  ai->idle_pos = SH / 2 - PH / 2;

  ai_left_state->rtime = 0.2f;
  ai_left_state->lreact = 0;
  ai_left_state->ty = *p1pos;
  ai_left_state->perror = 0;
  ai_left_state->conf = 0.8f;
  ai_left_state->active = false;
  ai_left_state->idle_pos = SH / 2 - PH / 2;

  game->p1 = 0;
  game->p2 = 0;
  game->rtimer = 0;
  game->waiting = false;
}

void ResetBall(Vector2 *bpos, Vector2 *bvel) {
  *bpos = (Vector2){SW / 2, SH / 2};

  float angle = (rand() % 91) - 45;
  if (rand() % 2 == 0)
    angle += 180;

  *bvel =
      (Vector2){BSPEED * cosf(angle * DEG2RAD), BSPEED * sinf(angle * DEG2RAD)};
}

float PredictBallY(Vector2 bpos, Vector2 bvel, AIState *ai, bool is_left) {
  float t_to_paddle;
  if (is_left)
    t_to_paddle = (bpos.x - PW) / -bvel.x;
  else
    t_to_paddle = (SW - PW - bpos.x) / bvel.x;

  float pred_y = bpos.y + (bvel.y * t_to_paddle) + ai->perror;

  if (GetRandomValue(0, 100) < 5)
    ai->perror = GetRandomValue(-30, 30) * (1.0f - ai->conf);

  return pred_y;
}

void UpdateAI(float *ppos, Vector2 bpos, Vector2 bvel, AIState *ai,
              bool is_left) {
  float time_now = GetTime();

  if ((is_left && bvel.x < 0) || (!is_left && bvel.x > 0)) {
    if (!ai->active) {
      ai->active = true;
      ai->lreact = time_now + ai->rtime;
    }
  } else {
    ai->active = false;
  }

  if (ai->active) {
    float dist_factor = 1.0f - fabsf((is_left ? 0 : SW) - bpos.x) / SW;
    ai->conf = 0.5f + dist_factor * 0.4f;

    if (time_now - ai->lreact > ai->rtime) {
      ai->ty = PredictBallY(bpos, bvel, ai, is_left);
      ai->lreact = time_now;

      if (GetRandomValue(0, 100) < 2)
        ai->rtime = 0.2f + (GetRandomValue(0, 100) / 100.0f) * 0.3f;
    }

    float paddle_center = *ppos + PH / 2;
    float dist = ai->ty - paddle_center;
    float speed_factor = fminf(fabsf(dist) / 100.0f, 1.0f);

    if (fabsf(dist) > 2.0f) {
      float move_speed = AISPEED * speed_factor * ai->conf;
      *ppos += (dist > 0 ? move_speed : -move_speed);
    }
  } else {
    float dist_to_idle = ai->idle_pos - *ppos;
    if (fabsf(dist_to_idle) > 1.0f)
      *ppos += (dist_to_idle > 0 ? 1 : -1) * (AISPEED * 0.5f);
  }
}

void CheckCollisions(Vector2 *bpos, Vector2 *bvel, float p1pos, float p2pos) {
  if (bpos->y - BRADIUS <= 0 || bpos->y + BRADIUS >= SH)
    bvel->y *= -1;

  if (bpos->x - BRADIUS <= PW && bpos->y >= p1pos && bpos->y <= p1pos + PH) {
    float relY = (p1pos + PH / 2) - bpos->y;
    float normRelY = relY / (PH / 2);
    float randVar = (float)GetRandomValue(-5, 5);
    float bounceAngle = (normRelY * 45.0f) + randVar;

    if (fabsf(bounceAngle) < 15.0f)
      bounceAngle = bounceAngle > 0 ? 15.0f : -15.0f;

    bounceAngle = Clamp(bounceAngle, -45.0f, 45.0f);

    bvel->x = BSPEED * cosf((90 - bounceAngle) * DEG2RAD);
    bvel->y = -BSPEED * sinf((90 - bounceAngle) * DEG2RAD);
  }

  if (bpos->x + BRADIUS >= SW - PW && bpos->y >= p2pos &&
      bpos->y <= p2pos + PH) {
    float relY = (p2pos + PH / 2) - bpos->y;
    float normRelY = relY / (PH / 2);
    float randVar = (float)GetRandomValue(-5, 5);
    float bounceAngle = (normRelY * 45.0f) + randVar;

    if (fabsf(bounceAngle) < 15.0f)
      bounceAngle = bounceAngle > 0 ? 15.0f : -15.0f;

    bounceAngle = Clamp(bounceAngle, -45.0f, 45.0f);

    bvel->x = -BSPEED * cosf((90 - bounceAngle) * DEG2RAD);
    bvel->y = -BSPEED * sinf((90 - bounceAngle) * DEG2RAD);
  }
}

void Update(Vector2 *bpos, Vector2 *bvel, float *p1pos, float *p2pos,
            bool *start, bool *ai_enabled, AIState *ai, AIState *ai_left_state,
            Game *game) {
  if (IsKeyPressed(KEY_P))
    *ai_enabled = !*ai_enabled;

  if (IsKeyPressed(KEY_L))
    ai_left = !ai_left;

  if (IsKeyPressed(KEY_SPACE))
    *start = true;

  if (game->waiting) {
    game->rtimer -= GetFrameTime();
    if (game->rtimer <= 0) {
      ResetBall(bpos, bvel);
      game->waiting = false;
    }
    return;
  }

  if (ai_left) {
    UpdateAI(p1pos, *bpos, *bvel, ai_left_state, true);
  } else {
    *p1pos += (IsKeyDown(KEY_S) - IsKeyDown(KEY_W)) * PSPEED;
  }

  if (*ai_enabled) {
    UpdateAI(p2pos, *bpos, *bvel, ai, false);
  } else {
    *p2pos += (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * PSPEED;
  }

  *p1pos = Clamp(*p1pos, 0, SH - PH);
  *p2pos = Clamp(*p2pos, 0, SH - PH);

  if (*start) {
    bpos->x += bvel->x;
    bpos->y += bvel->y;

    if (bpos->x < 0) {
      game->p2++;
      game->waiting = true;
      game->rtimer = RDELAY;
    } else if (bpos->x > SW) {
      game->p1++;
      game->waiting = true;
      game->rtimer = RDELAY;
    } else {
      CheckCollisions(bpos, bvel, *p1pos, *p2pos);
    }
  }
}

void Draw(Vector2 bpos, float p1pos, float p2pos, bool start, bool ai_enabled,
          Game game) {
  BeginDrawing();
  ClearBackground(BG);

  DrawCircle(bpos.x, bpos.y, BRADIUS, WHITE);
  DrawRectangle(0, p1pos, PW, PH, WHITE);
  DrawRectangle(SW - PW, p2pos, PW, PH, WHITE);

  char score_text[32];
  sprintf(score_text, "%d - %d", game.p1, game.p2);
  float score_width = MeasureText(score_text, 40);
  DrawText(score_text, SW / 2 - score_width / 2, 20, 40, WHITE);

  if (!start) {
    const char *text = "Press Space to Play";
    float textw = MeasureText(text, 30);
    DrawText(text, SW / 2 - textw / 2, SH / 2 - 60, 30, WHITE);
  }

  if (game.p1 >= MAXSCORE || game.p2 >= MAXSCORE) {
    const char *winner_text =
        game.p1 >= MAXSCORE ? "Player 1 Wins!" : "Player 2 Wins!";
    float winner_width = MeasureText(winner_text, 60);
    DrawText(winner_text, SW / 2 - winner_width / 2, SH / 2 - 30, 60, WHITE);
  }

  const char *ai_text =
      ai_enabled ? "AI: ON (P to toggle)" : "AI: OFF (P to toggle)";
  DrawText(ai_text, 10, 10, 20, WHITE);

  const char *ai_left_text =
      ai_left ? "AI Left: ON (L to toggle)" : "AI Left: OFF (L to toggle)";
  DrawText(ai_left_text, 10, 40, 20, WHITE);

  EndDrawing();
}

int main() {
  srand(time(0));
  InitWindow(SW, SH, "Pong");
  SetTargetFPS(60);

  Vector2 bpos, bvel;
  float p1pos, p2pos;
  bool start = false;
  bool ai_enabled = false;
  AIState ai, ai_left_state;
  Game game;

  Init(&bpos, &bvel, &p1pos, &p2pos, &ai_enabled, &ai, &ai_left_state, &game);

  while (!WindowShouldClose()) {
    if (game.p1 >= MAXSCORE || game.p2 >= MAXSCORE) {
      if (IsKeyPressed(KEY_R)) {
        start = false;
        Init(&bpos, &bvel, &p1pos, &p2pos, &ai_enabled, &ai, &ai_left_state,
             &game);
        continue;
      }
    }

    Update(&bpos, &bvel, &p1pos, &p2pos, &start, &ai_enabled, &ai,
           &ai_left_state, &game);
    Draw(bpos, p1pos, p2pos, start, ai_enabled, game);
  }

  CloseWindow();
  return 0;
}
