#include "FastLED.h"


#define DELAY      (15)
#define NUM_LEDS   (160)
#define NUM_STRIPS (1)

#define PIN_DATA   (19)
#define PIN_CLOCK  (18)

CRGB leds[NUM_STRIPS][NUM_LEDS];

void error(void)
{
    for (int i = 0; i < NUM_STRIPS; i++)
    {
        for (int j = 0; j < NUM_LEDS; j++)
        {
            leds[i][j].r = 0;
            leds[i][j].g = 255;
            leds[i][j].b = 0;
        }
    }
    LEDS.show();
}


typedef struct {
    unsigned int  y_pos;
    unsigned int  radius;
    unsigned int  r, g, b;
    unsigned int  target_r, target_g, target_b;
    unsigned long time_created;
    unsigned int  time_fade_in, time_live, time_fade_out;
} t_bubble;
#define ARRAY_LEN_LOG2  (6)
#define ARRAY_LEN       (1 << ARRAY_LEN_LOG2)
#define ARRAY_MOD_MASK  (ARRAY_LEN - 1)
t_bubble array[ARRAY_LEN];
unsigned int arr_start = 0, arr_size = 0;
unsigned long time_next_bubble = 0;
unsigned long time_last_iteration = 0;


int arr_has_space()
{
  return (arr_size < ARRAY_LEN);
}

unsigned int arr_next_idx()
{
  unsigned int idx = (arr_start + arr_size) & ARRAY_MOD_MASK;
  arr_size++;
  return idx;
}


int dead_bubble(unsigned int idx)
{
    t_bubble *b = &(array[idx]);
    unsigned long now = millis();
    unsigned long bubble_death = b->time_created + b->time_fade_in + b->time_live + b->time_fade_out;
    if (now >= bubble_death) return 1;
    return 0;
}

void arr_for_each(void (*func)(unsigned int))
{
  unsigned int leading_dead_bubbles = 0;
  for (unsigned int i = 0; i < arr_size; i++)
  {
    unsigned int idx = (arr_start + i) & ARRAY_MOD_MASK;
    func(idx);

    if (leading_dead_bubbles == i && dead_bubble(idx)) leading_dead_bubbles++;
  }

  if (leading_dead_bubbles > 0) {
      if (arr_size <= leading_dead_bubbles) { error(); return; }
      arr_start = (arr_start + leading_dead_bubbles) & ARRAY_MOD_MASK;
      arr_size -= leading_dead_bubbles;
  }
}

void new_bubble(unsigned int idx)
{
    t_bubble *b = &(array[idx]);
    memset(b, 0, sizeof(t_bubble));

    b->y_pos = random(NUM_LEDS);
    b->radius = 1 + random(9);
//    b->radius = 0;

    do {
        uint8_t channel = random(3);
        if (random(2)) b->target_r = random(255);
        if (random(2)) b->target_g = random(255);
        if (random(2)) b->target_b = random(255);
    } while (b->target_r == 0 && b->target_g == 0 && b->target_b == 0);

    b->time_created = millis();
    b->time_live = random(7000);
    b->time_fade_in  = 750 + random(5000);
    b->time_fade_out = 750 + random(5000);
}



void render_bubble(unsigned int idx)
{
    ////Serial.print("render_bubble\n");
    t_bubble *b = &(array[idx]);

    // Don't render empty bubbles
    if (b->r == 0 && b->g == 0 && b->b == 0) return;

    uint8_t y_min = b->y_pos - b->radius;
    if (y_min < 0) y_min = 0;

    uint8_t y_max = b->y_pos + b->radius;
    if (y_max >= NUM_LEDS) y_max = NUM_LEDS - 1;
    //Serial.print("render_bubble: idx="); //Serial.print(idx, DEC);
    //Serial.print(             "  radius="); //Serial.print(b->radius, DEC);
    //Serial.print("\n");
    for (int j = y_min; j <= y_max; j++)
    {
        //Serial.print("\tled_idx=");
        //Serial.print(j, DEC);
        //Serial.print("\n");

        uint16_t r = leds[0][j].r + b->r;
        if (r > 255) r = 255;
        leds[0][j].r = r;

        uint16_t g = leds[0][j].g + b->g;
        if (g > 255) g = 255;
        leds[0][j].g = g;

        uint16_t bl = leds[0][j].b + b->b;
        if (bl > 255) bl = 255;
        leds[0][j].b = bl;
    }
}

void render_bubbles()
{
    ////Serial.print("render_bubbles\n");
    memset(leds[0], 0,  NUM_LEDS * sizeof(struct CRGB));
    arr_for_each(&render_bubble);
}


void update_bubble(unsigned int idx)
{
    ////Serial.print("update_bubble\n");
    t_bubble *b = &(array[idx]);

    unsigned long now = millis();
    if (now < (b->time_created + b->time_fade_in)) {
      unsigned long time_fading_in = now - b->time_created;
      unsigned int pct_done = ((time_fading_in << 8) / b->time_fade_in);
      b->r = (b->target_r * pct_done) >> 8;
      b->g = (b->target_g * pct_done) >> 8;
      b->b = (b->target_b * pct_done) >> 8;
    } else if (now < (b->time_created + b->time_fade_in + b->time_live)) {
      b->r = b->target_r;
      b->g = b->target_g;
      b->b = b->target_b;
    } else if (now < (b->time_created + b->time_fade_in + b->time_live + b->time_fade_out)) {
      unsigned long time_fading_out = now - b->time_created - b->time_fade_in - b->time_live;
      unsigned int pct_done = 256 - ((time_fading_out << 8) / b->time_fade_out);
      b->r = (b->target_r * pct_done) >> 8;
      b->g = (b->target_g * pct_done) >> 8;
      b->b = (b->target_b * pct_done) >> 8;
    } else {
      b->r = b->g = b->b = 0;
    }
}


void update_bubbles()
{
    ////Serial.print("update_bubbles\n");
    arr_for_each(&update_bubble);
}

void setup()
{
    randomSeed(analogRead(0));
    LEDS.addLeds<LPD8806, PIN_DATA,  PIN_CLOCK>(leds[0],  NUM_LEDS);
    memset(leds[0], 0, NUM_LEDS*sizeof(CRGB));
    FastLED.setBrightness(64);
}

void loop()
{
/*
    unsigned long now = millis();
    unsigned long time_next_iteration = time_last_iteration + 10;
    Serial.print((long)time_next_iteration - (long)now, DEC);
    Serial.print("\n");
    if (time_next_iteration > now)
    {
        delay(time_next_iteration - now);
        return;
    }
    time_last_iteration = now;
*/
    update_bubbles();
    render_bubbles();
    LEDS.show();
    if (time_next_bubble < millis() && arr_has_space())
    {
        new_bubble(arr_next_idx());
        time_next_bubble = millis() + 500 + random(2000);
    }
}
