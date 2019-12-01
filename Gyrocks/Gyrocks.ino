/*
   Ported to Arduino Due by PhOBoZ
   
   Vector Game "Gyrocks" auf dem Oszilloskop
   Carsten Wartmann 2016/2017 cw@blenderbuch.de
   Fürs Make-Magazin

   Gehackt und basierend auf Trammel Hudsons Arbeit:

   Vector display using the MCP4921 DAC on the teensy3.1.
   More info: https://trmm.net/V.st
*/

/*
   Todo:
   - Alles auf ein System/Skalierung umstellen, fixe integer Mathe
   - implement Bounding Box Collision
   + Enemies und Rocks trennen
   + weniger Schüsse gleichzeitig
   - Enemy und Rocks können Ship schaden
   - Bug in Collisions Erkennung?
    - Enemies schießen
   - Lebenszähler für Ship
   + einfache Punktezählung
   - Feinde Formationen fliegen lassen
   - Alles auf Polarkoord umstellen (Ship/Bullets) für Kollisionsabfrage
   - Explosionen

*/

#include <math.h>

#include "GraphicsTranslator.h"
#include "hershey_font.h"
#include "objects.h"

//#define ENABLE_CONTROLS

// Sometimes the X and Y need to be flipped and/or swapped
//#define FLIP_X
//#define FLIP_Y
#define SLOW_MOVE

#define SIZE_SHIFT  2
#define BUFFER_SIZE 1024

#define DEFAULT_PEN   1
#define ROCK_PEN      2
#define ENEMY_PEN     3
#define SHIP_PEN      4
#define BULLET_PEN    5

/*  *********************** Game Stuff ***************************************************/

// Rock
typedef struct
{
  int16_t t = -1;
  int16_t r = 0;
  int16_t p = 0;
  int16_t x = 0;    // x/y Merker für Kollisionsabfrage
  int16_t y = 0;
  int16_t d = 0;
  int16_t vr;
  int16_t vp;
} rock_t;

// Enemy
typedef struct
{
  int16_t t = -1;
  int16_t r = 0;
  int16_t p = 0;
  int16_t vr;
  int16_t vp;
} enemy_t;

// Ship
typedef struct
{
  int16_t x = 1000;
  int16_t y = 1000;
  int16_t ax;
  int16_t ay;
  unsigned long firedelay;
} ship_t;

// Star
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t vx;
  int16_t vy;
  int16_t age;
} star_t;

// Bullet
typedef struct
{
  int16_t x;
  int16_t y;
  int16_t rot;
  int16_t vx;
  int16_t vy;
  int16_t age = -1;
} bullet_t;


#define HALT  // Auskommentieren um "Handbremse" für zweiten Knopf/Schalter zu lösen (Debug&Screenshot)

// Joystick
#define BUTT 14   // Digital
#define TRIG 15   // Digital
#define THRU 16    // Analog
#define POTX 17   // Analog
#define POTY 18    // Analog
#define DEADX 30  // Deadband X
#define DEADY 30  // Deadband Y

#define FIREDELAY 100   // Zweitverzögerung zwischen zwei Schüssen

// Hintergrundsterne
#define MAX_STARS 30
star_t s[MAX_STARS];

// max. Anzahl der Schüsse
#define MAX_BULLETS 5
bullet_t b[MAX_BULLETS];

// max. Zahl der Asteroiden/Rocks
#define MAX_ROCK 5
rock_t r[MAX_ROCK];

// max. Zahl der Feinde
#define MAX_ENEMY 5
enemy_t e[MAX_ENEMY];

// Infos zum Schiff/Ship speichern
ship_t ship;

// Current pen
int currPen = DEFAULT_PEN;

// Punktezähler
unsigned int score;

// Frames per Second/Framerate Merker
long fps;

// Schnelle aber ungenaue Sinus Tabelle
const  uint8_t isinTable8[] = {
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128,

  131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221,

  223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
};

// Schnelle aber ungenaue Sinus Funktion 
int isin(int x)
{
  boolean pos = true;  // positive - keeps an eye on the sign.
  uint8_t idx;
  // remove next 6 lines for fastest execution but without error/wraparound
  if (x < 0)
  {
    x = -x;
    pos = !pos;
  }
  if (x >= 360) x %= 360;
  if (x > 180)
  {
    idx = x - 180;
    pos = !pos;
  }
  else idx = x;
  if (idx > 90) idx = 180 - idx;
  if (pos) return isinTable8[idx] / 2 ;
  return -(isinTable8[idx] / 2);
}

// Cosinus
int icos(int x)
{
  return (isin(x + 90));
}




/* ************************************** DAC/vektor output stuff **************************************/

void moveto(int x, int y)
{
  uint16_t px, py;
  
  //Test!  Very stupid "Clipping"
  if (x >= 4096) x = 4095;
  if (y >= 4096) y = 4095;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  #ifdef FLIP_X
  px = 4095 - (x & 0xFFF);
#else
  px = x & 0xFFF;
#endif

#ifdef FLIP_Y
  py = 4095 - (y & 0xFFF);
#else
  py = y & 0xFFF;
#endif

  GraphicsTranslator.pen_enable(0);
  GraphicsTranslator.plot_absolute(px >> SIZE_SHIFT, py >> SIZE_SHIFT);
}


void lineto(int x, int y)
{
  uint16_t px, py;
  
  //Test!  Very stupid "Clipping"
  //if (x>=4096 ||x<0 ||y>4096 || y<0) return; //don't draw at all
  if (x >= 4096) x = 4095;
  if (y >= 4096) y = 4095;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

#ifdef FLIP_X
  px = 4095 - (x & 0xFFF);
#else
  px = x & 0xFFF;
#endif

#ifdef FLIP_Y
  py = 4095 - (y & 0xFFF);
#else
  py = y & 0xFFF;
#endif

  GraphicsTranslator.pen_enable(currPen);
  GraphicsTranslator.plot_absolute(px >> SIZE_SHIFT, py >> SIZE_SHIFT);

}


int draw_character(char c, int x, int y, int size)
{
  const hershey_char_t * const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -1)
    {
      next_moveto = 1;
      continue;
    }
    dx = (dx * size) * 3 / 4;
    dy = (dy * size) * 3 / 4; //??
    if (next_moveto)
      moveto(x + dx, y + dy);
    else
      lineto(x + dx, y + dy);
    next_moveto = 0;
  }
  return (f->width * size) * 3 / 4;
}


void draw_string(const char * s, int x, int y, int size)
{
  while (*s)
  {
    char c = *s++;
    x += draw_character(c, x, y, size);
  }
}



/* Setup all */
void setup()
{
  Serial.begin(9600); // baud rate is ignored  
  GraphicsTranslator.begin(BUFFER_SIZE);
  GraphicsTranslator.pen_RGB(ROCK_PEN, 0xB5, 0x65, 0x1D);
  GraphicsTranslator.pen_RGB(ENEMY_PEN, 0x00, 0xFF, 0x00);
  GraphicsTranslator.pen_RGB(SHIP_PEN, 0x00, 0x00, 0xFF);
  GraphicsTranslator.pen_RGB(BULLET_PEN, 0xFF, 0x00, 0x00);
#ifdef SLOW_MOVE
  GraphicsTranslator.interpolate_move = true;  
#else
  GraphicsTranslator.interpolate_move = false;  
#endif
  init_stars(s);
}



/* ***************************** Game Stuff **************************************************/

// Ähnlich draw_string aber mit definierter Rotation
void draw_object(byte c, int x, int y, int size, int rot)
{
  const objects_char_t * const f = &gobjects[c];

  int next_moveto = 1;
  int dxx, dyy;

  for (int i = 0 ; i < f->count ; i++)
  {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -127)
    {
      next_moveto = 1;
      continue;
    }
    dxx = ((dx * icos(rot)) - dy * isin(rot)) >> 7 ; // Würg irgendwie nicht Standard, KoordSys komisch?
    dyy = ((dy * icos(rot)) + dx * isin(rot)) >> 7 ;

    dx = (dxx * size) * 3 / 4 ;
    dy = (dyy * size) ;

    if (next_moveto)
      moveto(x + dx, y + dy);
    else
      lineto(x + dx, y + dy);
    next_moveto = 0;
  }
}


// Neuer Schuß wenn eine Slot frei (age==-1)
static void add_bullet(bullet_t * const bullets, ship_t * const ship, int rot)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age < 0)
    {
      b->x = ship->x;
      b->y = ship->y;

      b->rot = rot;
      b->vx = ship->ax ;
      b->vy = ship->ay ;
      b->age = 1;
      break;
    }
  }
}


// Updating bullets/Schüsse
static void update_bullets(bullet_t * const bullets)
{
  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    bullet_t * const b = &bullets[i];
    if (b->age >= 0)
    {
      if (b->age > 100 || (b->x > 2000 - 96 && b->x < 2000 + 96 && b->y > 2000 - 96 && b->y < 2000 + 96))
      {
        b->age = -1;
      }
      else
      {
        b->age++;
        b->x = b->x + (b->vx >> 1);
        b->y = b->y + (b->vy >> 1);
        draw_object(6, b->x, b->y, 10, b->rot);
      }
    }
  }
}


// Schiff Verwaltung
static void update_ship(ship_t * const ship)
{
  long d;
  int rot;

  d = ((2048 - ship->x) * (2048 - ship->x) + (2048 - ship->y) * (2048 - ship->y)) / 75000;
  if (d > 15) d = 15;
  if (d < 1) d = 1;

  if (collision_rock(ship->x, ship->y, d))
  {
    score = 0;
  }

  rot = atan2(2048 - ship->y, 2048 - ship->x) * 180.0 / PI - 90;  // different coord sys...?! Float... hmm

#ifdef ENABLE_CONTROLS
  // Fire
  if (!digitalRead(TRIG) == HIGH && millis() > (ship->firedelay + FIREDELAY))
  {
    ship->firedelay = millis();
    ship->ax = -isin(rot) >> 1  ;
    ship->ay =  icos(rot) >> 1  ;
    add_bullet(b, ship, rot);
  }
#endif
  draw_object(3, ship->x, ship->y, d, rot);               // Ship
  draw_object(4, ship->x, ship->y, d + rand() % d, rot);  // Engine
}


// Hintergrund
static void  update_stars(star_t * const stars)
{
  int age2;
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->age++;
    age2 = s->age * s->age >> 12;
    s->x = s->x + (s->vx * age2);
    s->y = s->y + (s->vy * age2);
    if (s->x > 4000 || s->x < 96 || s->y > 4000 || s->y < 96 || s->age > 200)
    {
      s->x = rand() % 50 + 2000;
      s->y = rand() % 50 + 2000;
      s->vx = rand() % 8 - 4 ;
      s->vy = rand() % 8 - 4 ;
      s->age = 0;
    }
    draw_character(43, s->x, s->y, age2 >> 1);  // Using a "+" char...
  }
}

static void  init_stars(star_t * const stars)
{
  for (uint8_t i = 0 ; i < MAX_STARS ; i++)
  {
    star_t * const s = &stars[i];
    s->x = rand() % 500 + 1750;
    s->y = rand() % 500 + 1750;

    s->vx = rand() % 8 - 4 ;
    s->vy = rand() % 8 - 4 ;
    s->age = rand() % 300;
  }
}


// Felsen/Rock/Asteroid
static void  add_rock(rock_t * const rock)
{
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    rock_t * const rr = &rock[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 2 + 13;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_rocks(rock_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    //rock_t * const rr = &rock[i];
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 30000 )
      {
        rr[i].r += rr[i].vr;
      }
      else
      {
        rr[i].t = -1;
        continue;
      }
      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;
      rr[i].x = x;
      rr[i].y = y;  // Keep track, x,y ToDo: raus oder auf Polarkoords
      rr[i].d = rr[i].r / 512;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 10;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].d, -rr[i].p / 4);
        draw_rect(r[i].x - r[i].d * 6, r[i].y - r[i].d * 6, r[i].x + r[i].d * 6, r[i].y + r[i].d * 6); // debug
      }
    }
  }
}


// Enemies
static void  add_enemy(enemy_t * const enemy)
{
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    enemy_t * const rr = &enemy[i];
    if (rr->t == -1)
    {
      rr->t = rand() % 4 + 18;
      rr->r = rand() % 1000 + 1;
      rr->p = rand() % 359 * 16;

      rr->vr = rand() % 30 + 15;
      rr->vp = rand() % 10 - 5  ;
      break;
    }
  }
}

static void  update_enemies(enemy_t * const rr)
{
  int x, y;
  for (uint8_t i = 0 ; i < MAX_ENEMY ; i++)
  {
    if (rr[i].t >= 0) // nur wenn live/type gesetzt
    {
      if (rr[i].r < 10000 )
      {
        rr[i].r += rr[i].vr;
      }

      rr[i].p = (rr[i].p + rr[i].vp) % (360 * 16) ;
      x = 2048 + (rr[i].r / 16 * icos(rr[i].p / 16)) / 100;
      y = 2048 + (rr[i].r / 16 * isin(rr[i].p / 16)) / 100;

      if (collision_bullet(x, y, rr[i].r / 512))
      {
        rr[i].t = -1;
        rr[i].r = 0;
        score += 100;
      }
      else
      {
        draw_object(rr[i].t, x, y, rr[i].r / 512, -rr[i].p / 4);
      }
    }
  }
}


// Irgndwie verallgemeinern?
int collision_rock(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d;
  y0 = y - d ;
  x1 = x + d;
  y1 = y + d ;

  draw_rect(x0, y0, x1, y1);
  for (uint8_t i = 0 ; i < MAX_ROCK ; i++)
  {
    if (r[i].t >= 0)
    {
      if (r[i].x > x0 && r[i].x < x1 && r[i].y > y0 && r[i].y < y1)
      {
        //        r[i].t = -1;    //Kill rock also?
        return 1; //Collision with Bullet
      }
    }
  }
  return 0; // No Collision
}



int collision_bullet(int x, int y, int d)
{
  int x0, y0, x1, y1;

  d = d * 10;
  x0 = x - d / 2;
  y0 = y - d / 2;
  x1 = x + d / 2;
  y1 = y + d / 2;

  for (uint8_t i = 0 ; i < MAX_BULLETS ; i++)
  {
    //    bullet_t * const b = &bullets[i];
    if (b[i].age >= 0)
    {
      if (b[i].x > x0 && b[i].x < x1 && b[i].y > y0 && b[i].y < y1)
      {
        b[i].age = -1;
        return 1; //Kollision mit Schuss/Bullet
      }
    }
  }
  return 0; // Keine Kollision
}


void draw_field()
{
#define CORNER 500
  moveto(0, CORNER);
  lineto(0, 0);
  lineto(CORNER, 0);

  moveto(4095 - CORNER, 0);
  lineto(4095, 0);
  lineto(4095, CORNER);

  moveto(4095, 4095 - CORNER);
  lineto(4095, 4095);
  lineto(4095 - CORNER, 4095);

  moveto(CORNER, 4095);
  lineto(0, 4095);
  lineto(0, 4095 - CORNER);
}


// Debug draw
void draw_rect(int x0, int y0, int x1, int y1)
{
  return;    //Debug!
  moveto(x0, y0);
  lineto(x1, y0);
  lineto(x1, y1);
  lineto(x0, y1);
  lineto(x0, y0);
}



// Anzeige Funktion
void video()
{
#ifdef ENABLE_CONTROLS
  // Joystick auslesen
  if (analogRead(POTX) > 512 + DEADX || analogRead(POTX) < 512 - DEADX)
  {
    ship.x = ship.x - (analogRead(POTX) - 512) / 4;
  }
  if (analogRead(POTY) > 512 + DEADY || analogRead(POTY) < 512 - DEADY)
  {
    ship.y = ship.y - (analogRead(POTY) - 512) / 4;
  }
#endif

  ship.x = constrain(ship.x, 400, 3700);
  ship.y = constrain(ship.y, 400, 3700);

  currPen = DEFAULT_PEN;
  update_stars(s);

  currPen = BULLET_PEN;
  update_bullets(b);

  currPen = ROCK_PEN;
  update_rocks(r);
  
  if (rand() % 500 == 1) add_rock(r);

  currPen = ENEMY_PEN;
  update_enemies(e);
  
  if (rand() % 500 == 1) add_enemy(e);

  currPen = SHIP_PEN;
  update_ship(&ship);

  currPen = DEFAULT_PEN;
  draw_field();
}



// Hauptfunktion
void loop() {
  long start_time = micros();

#ifdef ENABLE_CONTROLS
#undef HALT
#ifdef HALT
  // HALTing Game (Debug&Screenshot)
  if (!digitalRead(BUTT) == HIGH)
  {
#endif
#endif

    char buf[12];

    video();
    // Punktezähler ausgeben
    draw_string("Points:", 100, 150, 6);
    draw_string(itoa(score, buf, 10), 800, 150, 6);

    // FPS Todo: Debug Switch?!
    draw_string("FPS:", 3000, 150, 6);
    draw_string(itoa(fps, buf, 10), 3400, 150, 6);

#ifdef ENABLE_CONTROLS
#ifdef HALT
  }
#endif
#endif

  GraphicsTranslator.frame_end();
  fps = 1000000 / (micros() - start_time);
}
