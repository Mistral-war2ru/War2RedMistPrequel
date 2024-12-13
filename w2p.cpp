#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <ctime>
#include "patch.h" //Fois patcher
#include "defs.h"  //some addresses and constants
#include "names.h" //some constants
#include "chars.h" //font

bool game_started = false;

#define BUILD_QUEUE_SECTION 0x51425142//BQBQ
#define ORDER_QUEUE_SECTION 0x514F514F//OQOQ
byte build_queue[1600 * 2 * 8];
byte order_queue[1600 * 3 * 16];

#define RMPQ_SECTION 0x51504d52//RMPQ
byte names_secret[1600];

WORD m_screen_w = 640;
WORD m_screen_h = 480;
WORD m_minx = 176;
WORD m_miny = 16;
WORD m_maxx = 616;
WORD m_maxy = 456;
WORD m_added_height = m_screen_w - 480;
WORD m_added_width = m_screen_h - 640;
WORD m_align_y = 0;
WORD m_align_x = 0;

byte a_custom = 0;            // custom game
byte reg[4] = {0};            // region
int *unit[1610];              // array for units
int *unitt[1610];             // temp array for units
int units = 0;                // number of units in array
byte ua[255] = {255};         // attackers array
byte ut[255] = {255};         // targets array     only unit ua[i] can deal damage to unit ut[i]
byte m_devotion[256] = {255}; // units that have defence aura
bool first_step = false;      // first step of trigger
bool ai_fixed = false;        // ai fix
bool saveload_fixed = false;  // saveload ai break bug fix
bool A_portal = false;        // portals activated
bool A_autoheal = false;      // paladins autoheal activated
bool A_rally = false;         // rally point
bool A_tower = false;         // tower control

char churc[] = "\x0\x0\xa\x0\xc0\x43\x44\x0\x90\xe6\x40\x0\x14\x21\x72\x1\x0\x0\x0\x0\x0\x0\x6b\x0\x10\x44\x44\x0\x90\xe6\x40\x0\x1\x23\x81\x1\x0\x0\x0\x0\x1\x0\x6e\x0\x10\x44\x44\x0\x90\xe6\x40\x0\x3\x24\x83\x1\x0\x0\x0\x0\x3\x0\x6d\x0\xf0\x40\x44\x0\xf0\x40\x44\x0\x0\x0\x6c\x1\x0\x0\x0\x0";

struct Vizs
{
    byte x = 0;
    byte y = 0;
    byte p = 0;
    byte s = 0;
};
Vizs vizs_areas[2000];
int vizs_n = 0;

bool can_rain = false;
#define RAINDROPS 1000 // max amount
WORD raindrops_amount = RAINDROPS;
byte raindrops_density = 4;
byte raindrops_speed = 2;
byte raindrops_size = 8;
byte raindrops_size_x = 5; // cannot be more than raindrops_size
byte raindrops_size_y = 6; // cannot be more than raindrops_size
byte raindrops_align_x = raindrops_size - raindrops_size_x;
byte raindrops_align_y = raindrops_size - raindrops_size_y;
bool raindrops_snow = false;
#define THUNDER_GRADIENT 20
#define THUNDER_CHANGE 12
WORD raindrops_thunder = 0;
WORD raindrops_thunder_timer = 0;
WORD raindrops_thunder_gradient = THUNDER_GRADIENT;
#define RAINDROPS_COLOR 0xB
byte real_palette[256 * 4];

struct raindrop
{
    WORD x1 = 0;
    WORD y1 = 0;
    WORD x2 = 0;
    WORD y2 = 0;
    WORD l = 0;
};
raindrop raindrops[RAINDROPS];

#define my_min(x, y) (((x) < (y)) ? (x) : (y))
#define my_max(x, y) (((x) > (y)) ? (x) : (y))

void save_palette()
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            real_palette[i] = *(byte *)(ad + i);
        }
    }
}

void change_palette(bool a)
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            byte c = *(byte *)(ad + i);
            if (a)
            {
                if (c < (255 - THUNDER_CHANGE))
                    c += THUNDER_CHANGE;
                else
                    c = 255;
            }
            else
            {
                if (c > THUNDER_CHANGE)
                    c -= THUNDER_CHANGE;
                else
                    c = 0;
            }
            *(byte *)(ad + i) = c;
        }
    }
}

void reset_palette()
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            *(byte *)(ad + i) = real_palette[i];
        }
    }
}

//-----sound
void *def_name = NULL;
void *def_sound = NULL;
void *def_name_seq = NULL;
void *def_sound_seq = NULL;

char rally_name[] = "RedMistPrequel\\sounds\\rally.wav\x0";
void* rally_sound = NULL;
char rally2_name[] = "RedMistPrequel\\sounds\\rally2.wav\x0";
void* rally2_sound = NULL;

char assembly_name[] = "RedMistPrequel\\sounds\\assembly.wav\x0";
void* assembly_sound = NULL;

char farm_name[] = "RedMistPrequel\\sounds\\farm.wav\x0";
void* farm_sound = NULL;

char church_name[] = "RedMistPrequel\\sounds\\church.wav\x0";
void* church_sound = NULL;

char beer_name[] = "RedMistPrequel\\sounds\\beer.wav\x0";
void* beer_sound = NULL;

char resc_name[] = "RedMistPrequel\\sounds\\resc.wav\x0";
void* resc_sound = NULL;

char kir1_name[] = "RedMistPrequel\\kirka\\k1.wav\x0";
void *kir1_sound = NULL;
char kir2_name[] = "RedMistPrequel\\kirka\\k2.wav\x0";
void *kir2_sound = NULL;
char kir3_name[] = "RedMistPrequel\\kirka\\k3.wav\x0";
void *kir3_sound = NULL;
char kir4_name[] = "RedMistPrequel\\kirka\\k4.wav\x0";
void *kir4_sound = NULL;
char *kir_names[] = {kir1_name, kir2_name, kir3_name, kir4_name};
void *kir_sounds[] = {kir1_sound, kir2_sound, kir3_sound, kir4_sound};

char c1_name[] = "RedMistPrequel\\canon\\c1.wav\x0";
void *c1_sound = NULL;
char c2_name[] = "RedMistPrequel\\canon\\c2.wav\x0";
void *c2_sound = NULL;
char *c_names[] = {c1_name, c2_name};
void *c_sounds[] = {c1_sound, c2_sound};

char hy1_name[] = "RedMistPrequel\\speech\\comander\\dnyessr1.wav\x0";
void *hy1_sound = NULL;
char hy2_name[] = "RedMistPrequel\\speech\\comander\\dnyessr2.wav\x0";
void *hy2_sound = NULL;
char hy3_name[] = "RedMistPrequel\\speech\\comander\\dnyessr3.wav\x0";
void *hy3_sound = NULL;
char hw1_name[] = "RedMistPrequel\\speech\\comander\\dnwhat1.wav\x0";
void *hw1_sound = NULL;
char hw2_name[] = "RedMistPrequel\\speech\\comander\\dnwhat2.wav\x0";
void *hw2_sound = NULL;
char hw3_name[] = "RedMistPrequel\\speech\\comander\\dnwhat3.wav\x0";
void *hw3_sound = NULL;
char hp1_name[] = "RedMistPrequel\\speech\\comander\\dnpisd1.wav\x0";
void *hp1_sound = NULL;
char hp2_name[] = "RedMistPrequel\\speech\\comander\\dnpisd2.wav\x0";
void *hp2_sound = NULL;
char hp3_name[] = "RedMistPrequel\\speech\\comander\\dnpisd3.wav\x0";
void *hp3_sound = NULL;
char *h_names[] = {hp1_name, hp2_name, hp3_name, hw1_name, hw2_name, hw3_name, hy1_name, hy2_name, hy3_name};
void *h_sounds[] = {hp1_sound, hp2_sound, hp3_sound, hw1_sound, hw2_sound, hw3_sound, hy1_sound, hy2_sound, hy3_sound};

char wy1_name[] = "RedMistPrequel\\speech\\worker\\wyessr1.wav\x0";
void *wy1_sound = NULL;
char wy2_name[] = "RedMistPrequel\\speech\\worker\\wyessr2.wav\x0";
void *wy2_sound = NULL;
char wy3_name[] = "RedMistPrequel\\speech\\worker\\wyessr3.wav\x0";
void *wy3_sound = NULL;
char wy4_name[] = "RedMistPrequel\\speech\\worker\\wyessr4.wav\x0";
void *wy4_sound = NULL;
char ww1_name[] = "RedMistPrequel\\speech\\worker\\wwhat1.wav\x0";
void *ww1_sound = NULL;
char ww2_name[] = "RedMistPrequel\\speech\\worker\\wwhat2.wav\x0";
void *ww2_sound = NULL;
char ww3_name[] = "RedMistPrequel\\speech\\worker\\wwhat3.wav\x0";
void *ww3_sound = NULL;
char ww4_name[] = "RedMistPrequel\\speech\\worker\\wwhat4.wav\x0";
void *ww4_sound = NULL;
char wp1_name[] = "RedMistPrequel\\speech\\worker\\wpissd1.wav\x0";
void *wp1_sound = NULL;
char wp2_name[] = "RedMistPrequel\\speech\\worker\\wpissd2.wav\x0";
void *wp2_sound = NULL;
char wp3_name[] = "RedMistPrequel\\speech\\worker\\wpissd3.wav\x0";
void *wp3_sound = NULL;
char wp4_name[] = "RedMistPrequel\\speech\\worker\\wpissd4.wav\x0";
void *wp4_sound = NULL;
char wp5_name[] = "RedMistPrequel\\speech\\worker\\wpissd5.wav\x0";
void *wp5_sound = NULL;
char wp6_name[] = "RedMistPrequel\\speech\\worker\\wpissd6.wav\x0";
void *wp6_sound = NULL;
char wp7_name[] = "RedMistPrequel\\speech\\worker\\wpissd7.wav\x0";
void *wp7_sound = NULL;
char wr_name[] = "RedMistPrequel\\speech\\worker\\wready.wav\x0";
void *wr_sound = NULL;
char wd_name[] = "RedMistPrequel\\speech\\worker\\wrkdon.wav\x0";
void *wd_sound = NULL;
char *w_names[] = {wr_name, wp1_name, wp2_name, wp3_name, wp4_name, wp5_name, wp6_name, wp7_name, ww1_name, ww2_name, ww3_name, ww4_name, wy1_name, wy2_name, wy3_name, wy4_name, wd_name};
void *w_sounds[] = {wr_sound, wp1_sound, wp2_sound, wp3_sound, wp4_sound, wp5_sound, wp6_sound, wp7_sound, ww1_sound, ww2_sound, ww3_sound, ww4_sound, wy1_sound, wy2_sound, wy3_sound, wy4_sound, wd_sound};

char huy1_name[] = "RedMistPrequel\\speech\\human\\Hyessir1.wav\x0";
void *huy1_sound = NULL;
char huy2_name[] = "RedMistPrequel\\speech\\human\\Hyessir2.wav\x0";
void *huy2_sound = NULL;
char huy3_name[] = "RedMistPrequel\\speech\\human\\Hyessir3.wav\x0";
void *huy3_sound = NULL;
char huy4_name[] = "RedMistPrequel\\speech\\human\\Hyessir4.wav\x0";
void *huy4_sound = NULL;
char huw1_name[] = "RedMistPrequel\\speech\\human\\Hwhat1.wav\x0";
void *huw1_sound = NULL;
char huw2_name[] = "RedMistPrequel\\speech\\human\\Hwhat2.wav\x0";
void *huw2_sound = NULL;
char huw3_name[] = "RedMistPrequel\\speech\\human\\Hwhat3.wav\x0";
void *huw3_sound = NULL;
char huw4_name[] = "RedMistPrequel\\speech\\human\\Hwhat4.wav\x0";
void *huw4_sound = NULL;
char huw5_name[] = "RedMistPrequel\\speech\\human\\Hwhat5.wav\x0";
void *huw5_sound = NULL;
char huw6_name[] = "RedMistPrequel\\speech\\human\\Hwhat6.wav\x0";
void *huw6_sound = NULL;
char hup1_name[] = "RedMistPrequel\\speech\\human\\Hpissed1.wav\x0";
void *hup1_sound = NULL;
char hup2_name[] = "RedMistPrequel\\speech\\human\\Hpissed2.wav\x0";
void *hup2_sound = NULL;
char hup3_name[] = "RedMistPrequel\\speech\\human\\Hpissed3.wav\x0";
void *hup3_sound = NULL;
char hup4_name[] = "RedMistPrequel\\speech\\human\\Hpissed4.wav\x0";
void *hup4_sound = NULL;
char hup5_name[] = "RedMistPrequel\\speech\\human\\Hpissed5.wav\x0";
void *hup5_sound = NULL;
char hup6_name[] = "RedMistPrequel\\speech\\human\\Hpissed6.wav\x0";
void *hup6_sound = NULL;
char hup7_name[] = "RedMistPrequel\\speech\\human\\Hpissed7.wav\x0";
void *hup7_sound = NULL;
char hur_name[] = "RedMistPrequel\\speech\\human\\Hready.wav\x0";
void *hur_sound = NULL;
char wdn_name[] = "RedMistPrequel\\speech\\human\\wrkdone.wav\x0";
void *wdn_sound = NULL;
char *hu_names[] = {huw1_name, huw2_name, huw3_name, huw4_name, huw5_name, huw6_name, hup1_name, hup2_name, hup3_name, hup4_name, hup5_name, hup6_name, hup7_name, hup7_name, huy1_name, huy2_name, huy3_name, huy4_name, huy4_name, huy4_name, hur_name, wdn_name};
void *hu_sounds[] = {huw1_sound, huw2_sound, huw3_sound, huw4_sound, huw5_sound, huw6_sound, hup1_sound, hup2_sound, hup3_sound, hup4_sound, hup5_sound, hup6_sound, hup7_sound, hup7_sound, huy1_sound, huy2_sound, huy3_sound, huy4_sound, huy4_sound, huy4_sound, hur_sound, wdn_sound};

char hh1_name[] = "RedMistPrequel\\speech\\human\\help1.wav\x0";
void *hh1_sound = NULL;
char hh2_name[] = "RedMistPrequel\\speech\\human\\help2.wav\x0";
void *hh2_sound = NULL;

char ky1_name[] = "RedMistPrequel\\speech\\armored\\ayessr1.wav\x0";
void *ky1_sound = NULL;
char ky2_name[] = "RedMistPrequel\\speech\\armored\\ayessr2.wav\x0";
void *ky2_sound = NULL;
char ky3_name[] = "RedMistPrequel\\speech\\armored\\ayessr3.wav\x0";
void *ky3_sound = NULL;
char ky4_name[] = "RedMistPrequel\\speech\\armored\\ayessr4.wav\x0";
void *ky4_sound = NULL;
char kw1_name[] = "RedMistPrequel\\speech\\armored\\awhat1.wav\x0";
void *kw1_sound = NULL;
char kw2_name[] = "RedMistPrequel\\speech\\armored\\awhat2.wav\x0";
void *kw2_sound = NULL;
char kw3_name[] = "RedMistPrequel\\speech\\armored\\awhat3.wav\x0";
void *kw3_sound = NULL;
char kw4_name[] = "RedMistPrequel\\speech\\armored\\awhat4.wav\x0";
void *kw4_sound = NULL;
char kp1_name[] = "RedMistPrequel\\speech\\armored\\apissd1.wav\x0";
void *kp1_sound = NULL;
char kp2_name[] = "RedMistPrequel\\speech\\armored\\apissd2.wav\x0";
void *kp2_sound = NULL;
char kp3_name[] = "RedMistPrequel\\speech\\armored\\apissd3.wav\x0";
void *kp3_sound = NULL;
char kr_name[] = "RedMistPrequel\\speech\\armored\\aready.wav\x0";
void *kr_sound = NULL;
char *k_names[] = {kp1_name, kp2_name, kp3_name, kr_name, kw1_name, kw2_name, kw3_name, kw4_name, ky1_name, ky2_name, ky3_name, ky4_name};
void *k_sounds[] = {kp1_sound, kp2_sound, kp3_sound, kr_sound, kw1_sound, kw2_sound, kw3_sound, kw4_sound, ky1_sound, ky2_sound, ky3_sound, ky4_sound};

char pky1_name[] = "RedMistPrequel\\speech\\paladin\\Pkyessr1.wav\x0";
void *pky1_sound = NULL;
char pky2_name[] = "RedMistPrequel\\speech\\paladin\\Pkyessr2.wav\x0";
void *pky2_sound = NULL;
char pky3_name[] = "RedMistPrequel\\speech\\paladin\\Pkyessr3.wav\x0";
void *pky3_sound = NULL;
char pky4_name[] = "RedMistPrequel\\speech\\paladin\\Pkyessr4.wav\x0";
void *pky4_sound = NULL;
char pkw1_name[] = "RedMistPrequel\\speech\\paladin\\Pkwhat1.wav\x0";
void *pkw1_sound = NULL;
char pkw2_name[] = "RedMistPrequel\\speech\\paladin\\Pkwhat2.wav\x0";
void *pkw2_sound = NULL;
char pkw3_name[] = "RedMistPrequel\\speech\\paladin\\Pkwhat3.wav\x0";
void *pkw3_sound = NULL;
char pkw4_name[] = "RedMistPrequel\\speech\\paladin\\Pkwhat4.wav\x0";
void *pkw4_sound = NULL;
char pkp1_name[] = "RedMistPrequel\\speech\\paladin\\Pkpissd1.wav\x0";
void *pkp1_sound = NULL;
char pkp2_name[] = "RedMistPrequel\\speech\\paladin\\Pkpissd2.wav\x0";
void *pkp2_sound = NULL;
char pkp3_name[] = "RedMistPrequel\\speech\\paladin\\Pkpissd3.wav\x0";
void *pkp3_sound = NULL;
char pkr_name[] = "RedMistPrequel\\speech\\paladin\\Pkready.wav\x0";
void *pkr_sound = NULL;
char *pk_names[] = {pkp1_name, pkp2_name, pkp3_name, pkr_name, pkw1_name, pkw2_name, pkw3_name, pkw4_name, pky1_name, pky2_name, pky3_name, pky4_name};
void *pk_sounds[] = {pkp1_sound, pkp2_sound, pkp3_sound, pkr_sound, pkw1_sound, pkw2_sound, pkw3_sound, pkw4_sound, pky1_sound, pky2_sound, pky3_sound, pky4_sound};

char ey1_name[] = "RedMistPrequel\\speech\\elves\\Eyessir1.wav\x0";
void *ey1_sound = NULL;
char ey2_name[] = "RedMistPrequel\\speech\\elves\\Eyessir2.wav\x0";
void *ey2_sound = NULL;
char ey3_name[] = "RedMistPrequel\\speech\\elves\\Eyessir3.wav\x0";
void *ey3_sound = NULL;
char ey4_name[] = "RedMistPrequel\\speech\\elves\\Eyessir4.wav\x0";
void *ey4_sound = NULL;
char ew1_name[] = "RedMistPrequel\\speech\\elves\\Ewhat1.wav\x0";
void *ew1_sound = NULL;
char ew2_name[] = "RedMistPrequel\\speech\\elves\\Ewhat2.wav\x0";
void *ew2_sound = NULL;
char ew3_name[] = "RedMistPrequel\\speech\\elves\\Ewhat3.wav\x0";
void *ew3_sound = NULL;
char ew4_name[] = "RedMistPrequel\\speech\\elves\\Ewhat4.wav\x0";
void *ew4_sound = NULL;
char ep1_name[] = "RedMistPrequel\\speech\\elves\\Epissed1.wav\x0";
void *ep1_sound = NULL;
char ep2_name[] = "RedMistPrequel\\speech\\elves\\Epissed2.wav\x0";
void *ep2_sound = NULL;
char ep3_name[] = "RedMistPrequel\\speech\\elves\\Epissed3.wav\x0";
void *ep3_sound = NULL;
char er_name[] = "RedMistPrequel\\speech\\elves\\Eready.wav\x0";
void *er_sound = NULL;
char *e_names[] = {ep1_name, ep2_name, ep3_name, er_name, ew1_name, ew2_name, ew3_name, ew4_name, ey1_name, ey2_name, ey3_name, ey4_name};
void *e_sounds[] = {ep1_sound, ep2_sound, ep3_sound, er_sound, ew1_sound, ew2_sound, ew3_sound, ew4_sound, ey1_sound, ey2_sound, ey3_sound, ey4_sound};

char sy1_name[] = "RedMistPrequel\\speech\\ships\\Hshpyes1.wav\x0";
void *sy1_sound = NULL;
char sy2_name[] = "RedMistPrequel\\speech\\ships\\Hshpyes2.wav\x0";
void *sy2_sound = NULL;
char sy3_name[] = "RedMistPrequel\\speech\\ships\\Hshpyes3.wav\x0";
void *sy3_sound = NULL;
char sw1_name[] = "RedMistPrequel\\speech\\ships\\Hshpwht1.wav\x0";
void *sw1_sound = NULL;
char sw2_name[] = "RedMistPrequel\\speech\\ships\\Hshpwht2.wav\x0";
void *sw2_sound = NULL;
char sw3_name[] = "RedMistPrequel\\speech\\ships\\Hshpwht3.wav\x0";
void *sw3_sound = NULL;
char sp1_name[] = "RedMistPrequel\\speech\\ships\\Hshppis1.wav\x0";
void *sp1_sound = NULL;
char sp2_name[] = "RedMistPrequel\\speech\\ships\\Hshppis2.wav\x0";
void *sp2_sound = NULL;
char sp3_name[] = "RedMistPrequel\\speech\\ships\\Hshppis3.wav\x0";
void *sp3_sound = NULL;
char sr_name[] = "RedMistPrequel\\speech\\ships\\Hshpredy.wav\x0";
void *sr_sound = NULL;
char *s_names[] = {sp1_name, sp2_name, sp3_name, sr_name, sw1_name, sw2_name, sw3_name, sy1_name, sy2_name, sy3_name};
void *s_sounds[] = {sp1_sound, sp2_sound, sp3_sound, sr_sound, sw1_sound, sw2_sound, sw3_sound, sy1_sound, sy2_sound, sy3_sound};

char dy1_name[] = "RedMistPrequel\\speech\\dwarf\\Dwyessr1.wav\x0";
void *dy1_sound = NULL;
char dy2_name[] = "RedMistPrequel\\speech\\dwarf\\Dwyessr2.wav\x0";
void *dy2_sound = NULL;
char dy3_name[] = "RedMistPrequel\\speech\\dwarf\\Dwyessr3.wav\x0";
void *dy3_sound = NULL;
char dy4_name[] = "RedMistPrequel\\speech\\dwarf\\Dwyessr4.wav\x0";
void *dy4_sound = NULL;
char dy5_name[] = "RedMistPrequel\\speech\\dwarf\\Dwyessr5.wav\x0";
void *dy5_sound = NULL;
char dw1_name[] = "RedMistPrequel\\speech\\dwarf\\Dwhat1.wav\x0";
void *dw1_sound = NULL;
char dw2_name[] = "RedMistPrequel\\speech\\dwarf\\Dwhat2.wav\x0";
void *dw2_sound = NULL;
char dp1_name[] = "RedMistPrequel\\speech\\dwarf\\Dwpissd1.wav\x0";
void *dp1_sound = NULL;
char dp2_name[] = "RedMistPrequel\\speech\\dwarf\\Dwpissd2.wav\x0";
void *dp2_sound = NULL;
char dp3_name[] = "RedMistPrequel\\speech\\dwarf\\Dwpissd3.wav\x0";
void *dp3_sound = NULL;
char dr_name[] = "RedMistPrequel\\speech\\dwarf\\Dwready.wav\x0";
void *dr_sound = NULL;
char dd_name[] = "RedMistPrequel\\speech\\human\\dead.wav\x0";
void *dd_sound = NULL;
char *d_names[] = {dp1_name, dp2_name, dp3_name, dr_name, dw1_name, dw2_name, dy1_name, dy2_name, dy3_name, dy4_name, dy5_name, dd_name};
void *d_sounds[] = {dp1_sound, dp2_sound, dp3_sound, dr_sound, dw1_sound, dw2_sound, dy1_sound, dy2_sound, dy3_sound, dy4_sound, dy5_sound, dd_sound};

char gy1_name[] = "RedMistPrequel\\speech\\gnome\\Gnyessr1.wav\x0";
void *gy1_sound = NULL;
char gp1_name[] = "RedMistPrequel\\speech\\gnome\\Gnpissd1.wav\x0";
void *gp1_sound = NULL;
char gp2_name[] = "RedMistPrequel\\speech\\gnome\\Gnpissd2.wav\x0";
void *gp2_sound = NULL;
char gp3_name[] = "RedMistPrequel\\speech\\gnome\\Gnpissd3.wav\x0";
void *gp3_sound = NULL;
char gp4_name[] = "RedMistPrequel\\speech\\gnome\\Gnpissd4.wav\x0";
void *gp4_sound = NULL;
char gp5_name[] = "RedMistPrequel\\speech\\gnome\\Gnpissd5.wav\x0";
void *gp5_sound = NULL;
char gr_name[] = "RedMistPrequel\\speech\\gnome\\Gnready.wav\x0";
void *gr_sound = NULL;
char *g_names[] = {gp1_name, gp2_name, gp3_name, gp4_name, gp5_name, gr_name, gy1_name};
void *g_sounds[] = {
    gp1_sound,
    gp2_sound,
    gp3_sound,
    gp4_sound,
    gp5_sound,
    gr_sound,
    gy1_sound,
};

void sound_play_from_file(int oid, DWORD name, void* &snd) // old id
{
    def_name = (void *)*(int *)(SOUNDS_FILES_LIST + 8 + 24 * oid);
    def_sound = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid); // save default
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)name);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)snd);
    ((void (*)(WORD))F_WAR2_SOUND_PLAY)(oid); // war2 sound play
    snd = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)def_sound);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)def_name); // restore default
}
//-----sound

#define KEY_ID_SHIFT 16
bool check_key_pressed(int id)
{
    return ((bool (*)(int))0x0048A850)(id);
}

void inval_game_view()
{
    ((void (*)(int, int, int, int))F_INVALIDATE)(m_minx, m_miny, m_maxx + 8, m_maxy + 8);
}

void show_message(byte time, char* text)
{
    ((void (*)(char*, int, int))F_MAP_MSG)(text, 15, time * 10); // original war2 show msg func
}

int get_val(int adress, int player)
{
    return (int)(*(WORD *)(adress + player * 2)); // player*2 cause all vals is WORD
}

WORD unit_fixup(int* u)
{
    //return ((WORD(*)(int*, byte))F_UNIT_FIXUP)(u, 1);//orig fixup save

    return (WORD)(((DWORD)u - (DWORD)(int*)(*(int*)UNITS_MASSIVE)) / 152);//152 unit size
}

int* unit_unfixup(WORD u)
{
    //return ((int* (*)(DWORD, byte))F_UNIT_FIXUP)(u, 0);//orig fixup load

    return (int*)(u * 152 + (DWORD)(int*)(*(int*)UNITS_MASSIVE));
}

bool order_type_target(byte o)
{
    return ((o == ORDER_FOLLOW) || (o == ORDER_FOLLOW_GUARD) || (o == ORDER_ATTACK_TARGET) || (o == ORDER_DEMOLISH_NEAR) ||
        (o == ORDER_REPAIR) || (o == ORDER_ENTER_TRANSPORT) || (o == ORDER_SPELL_HEAL) || (o == ORDER_SPELL_FIRESHIELD) ||
        (o == ORDER_SPELL_FIREBALL) || (o == ORDER_SPELL_INVIS) || (o == ORDER_SPELL_POLYMORPH) || (o == ORDER_SPELL_BLOODLUST) ||
        (o == ORDER_SPELL_HASTE) || (o == ORDER_SPELL_ARMOR));
}

bool cmp_args(byte m, byte v, byte c)
{ // compare bytes
    bool f = false;
    switch (m)
    {
    case CMP_EQ:
        f = (v == c);
        break;
    case CMP_NEQ:
        f = (v != c);
        break;
    case CMP_BIGGER_EQ:
        f = (v >= c);
        break;
    case CMP_SMALLER_EQ:
        f = (v <= c);
        break;
    case CMP_BIGGER:
        f = (v > c);
        break;
    case CMP_SMALLER:
        f = (v < c);
        break;
    default:
        f = false;
        break;
    }
    return f;
}

bool cmp_args2(byte m, WORD v, WORD c)
{ // compare words
    bool f = false;
    switch (m)
    {
    case CMP_EQ:
        f = (v == c);
        break;
    case CMP_NEQ:
        f = (v != c);
        break;
    case CMP_BIGGER_EQ:
        f = (v >= c);
        break;
    case CMP_SMALLER_EQ:
        f = (v <= c);
        break;
    case CMP_BIGGER:
        f = (v > c);
        break;
    case CMP_SMALLER:
        f = (v < c);
        break;
    default:
        f = false;
        break;
    }
    return f;
}

bool cmp_args4(byte m, int v, int c)
{ // comapre 4 bytes (for resources)
    bool f = false;
    switch (m)
    {
    case CMP_EQ:
        f = (v == c);
        break;
    case CMP_NEQ:
        f = (v != c);
        break;
    case CMP_BIGGER_EQ:
        f = (v >= c);
        break;
    case CMP_SMALLER_EQ:
        f = (v <= c);
        break;
    case CMP_BIGGER:
        f = (v > c);
        break;
    case CMP_SMALLER:
        f = (v < c);
        break;
    default:
        f = false;
        break;
    }
    return f;
}

void lose(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\x0"; // if need to show table
        PATCH_SET((char *)LOSE_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x3b";
        PATCH_SET((char *)LOSE_SHOW_TABLE, buf);
    }
    if (!first_step)
    {
        char l[] = "\x2";
        PATCH_SET((char *)(ENDGAME_STATE + (*(byte *)LOCAL_PLAYER)), l);
        ((void (*)())F_LOSE)(); // original lose func
    }
    else
    {
        patch_setdword((DWORD *)VICTORY_FUNCTION, (DWORD)F_LOSE);
    }
}

void win(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\xEB"; // if need to show table
        PATCH_SET((char *)WIN_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x74";
        PATCH_SET((char *)WIN_SHOW_TABLE, buf);
    }
    if (!first_step)
    {
        char l[] = "\x3";
        PATCH_SET((char *)(ENDGAME_STATE + (*(byte *)LOCAL_PLAYER)), l);
        ((void (*)())F_WIN)(); // original win func
    }
    else
    {
        patch_setdword((DWORD *)VICTORY_FUNCTION, (DWORD)F_WIN);
    }
}

void tile_remove_trees(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_TREES)(x, y);
}

void tile_remove_rocks(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_ROCKS)(x, y);
}

void tile_remove_walls(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_WALLS)(x, y);
}

bool stat_byte(byte s)
{ // chech if unit stat is 1 or 2 byte
    bool f = (s == S_DRAW_X) || (s == S_DRAW_Y) || (s == S_X) || (s == S_Y) || (s == S_HP) || (s == S_INVIZ) || (s == S_SHIELD) || (s == S_BLOOD) || (s == S_HASTE) || (s == S_AI_SPELLS) || (s == S_NEXT_FIRE) || (s == S_LAST_HARVEST_X) || (s == S_LAST_HARVEST_Y) || (s == S_BUILD_PROGRES) || (s == S_BUILD_PROGRES_TOTAL) || (s == S_RESOURCES) || (s == S_ORDER_X) || (s == S_ORDER_Y) || (s == S_RETARGET_X1) || (s == S_RETARGET_Y1) || (s == S_RETARGET_X2) || (s == S_RETARGET_Y2);
    return !f;
}

bool cmp_stat(int *p, int v, byte pr, byte cmp)
{
    // p - unit
    // v - value
    // pr - property
    // cmp - compare method
    bool f = false;
    if (stat_byte(pr))
    {
        byte ob = v % 256;
        char buf[3] = {0};
        buf[0] = ob;
        buf[1] = *((byte *)((uintptr_t)p + pr));
        if (cmp_args(cmp, buf[1], buf[0]))
        {
            f = true;
        }
    }
    else
    {
        if (cmp_args2(cmp, *((WORD *)((uintptr_t)p + pr)), (WORD)v))
        {
            f = true;
        }
    }
    return f;
}

void set_stat(int *p, int v, byte pr)
{
    if (stat_byte(pr))
    {
        char buf[] = "\x0";
        buf[0] = v % 256;
        PATCH_SET((char *)((uintptr_t)p + pr), buf);
    }
    else
    {
        char buf[] = "\x0\x0";
        buf[0] = v % 256;
        buf[1] = (v / 256) % 256;
        PATCH_SET((char *)((uintptr_t)p + pr), buf);
    }
}

void unit_convert(byte player, int who, int tounit, int a)
{
    // original war2 func converts units
    ((void (*)(byte, int, int, int))F_UNIT_CONVERT)(player, who, tounit, a);
}

void unit_create(int x, int y, int id, byte owner, byte n)
{
    while (n > 0)
    {
        n -= 1;
        int *p = (int *)(UNIT_SIZE_TABLE + 4 * id);                                // unit sizes table
        ((void (*)(int, int, int, byte, int *))F_UNIT_CREATE)(x, y, id, owner, p); // original war2 func creates unit
        // just call n times to create n units
    }
}

void unit_kill(int *u)
{
    ((void (*)(int *))F_UNIT_KILL)(u); // original war2 func kills unit
}

void unit_remove(int *u)
{
    byte f = *((byte *)((uintptr_t)u + S_FLAGS3));
    f |= SF_HIDDEN;
    set_stat(u, f, S_FLAGS3);
    unit_kill(u); // hide unit then kill
}

void unit_cast(int *u) // unit autocast
{
    ((void (*)(int *))F_AI_CAST)(u); // original war2 ai cast spells
}

int *bullet_create(WORD x, WORD y, byte id)
{
    int *b = ((int *(*)(WORD, WORD, byte))F_BULLET_CREATE)(x, y, id);
    if (b)
    {
        if ((id == B_LIGHT_FIRE) || (id == B_HEAVY_FIRE))
        {
            char buf[] = "\x0";
            buf[0] = 5;                                  // bullet action
            PATCH_SET((char *)((uintptr_t)b + 54), buf); // 54 bullet action
            buf[0] = 1;                                  // bullet info
            PATCH_SET((char *)((uintptr_t)b + 58), buf); // 58 bullet user info
            buf[0] = 4;                                  // bullet flags
            PATCH_SET((char *)((uintptr_t)b + 53), buf); // 53 bullet flags
            buf[0] = 80;                                 // ticks
            PATCH_SET((char *)((uintptr_t)b + 56), buf); // 56 bullet life (WORD)
        }
    }
    return b;
}

void bullet_create_unit(int *u, byte b)
{
    WORD x = *((WORD *)((uintptr_t)u + S_DRAW_X));
    WORD y = *((WORD *)((uintptr_t)u + S_DRAW_Y));
    bullet_create(x + 16, y + 16, b);
}

void bullet_create8_around_unit(int *u, byte b)
{
    WORD ux = *((WORD *)((uintptr_t)u + S_DRAW_X));
    WORD uy = *((WORD *)((uintptr_t)u + S_DRAW_Y));
    WORD x = ux + 16;
    WORD y = uy + 16;
    if ((b == B_LIGHT_FIRE) || (b == B_HEAVY_FIRE))
        y -= 8;
    if ((b == B_LIGHTNING) || (b == B_COIL))
    {
        x += 16;
        y += 16;
    }
    WORD xx = x;
    WORD yy = y;
    bullet_create(xx + 48, yy, b);      // right
    bullet_create(xx, yy + 48, b);      // down
    bullet_create(xx + 32, yy + 32, b); // right down
    if (xx <= 32)
        xx = 0;
    else
        xx -= 32;
    bullet_create(xx, yy + 32, b); // left down
    if (yy <= 32)
        yy = 0;
    else
        yy -= 32;
    bullet_create(xx, yy, b); // left up
    xx = x;
    bullet_create(xx + 32, yy, b); // right up
    yy = y;
    if (xx <= 48)
        xx = 0;
    else
        xx -= 48;
    bullet_create(xx, yy, b); // left
    xx = x;
    if (yy <= 48)
        yy = 0;
    else
        yy -= 48;
    bullet_create(xx, yy, b); // up
}

void set_region(int x1, int y1, int x2, int y2)
{
    if (x1 < 0)
        x1 = 0;
    if (x1 > 127)
        x1 = 127;
    if (y1 < 0)
        y1 = 0;
    if (y1 > 127)
        y1 = 127;
    if (x2 < 0)
        x2 = 0;
    if (x2 > 127)
        x2 = 127;
    if (y2 < 0)
        y2 = 0;
    if (y2 > 127)
        y2 = 127;
    reg[0] = x1 % 256;
    reg[1] = y1 % 256;
    reg[2] = x2 % 256;
    reg[3] = y2 % 256;
}

bool in_region(byte x, byte y, byte x1, byte y1, byte x2, byte y2)
{
    // dnt know why but without this big monstrous ussless code gam crash
    byte tmp;
    tmp = x % 256;
    x = tmp;
    tmp = y % 256;
    y = tmp;
    tmp = x1 % 256;
    x1 = tmp;
    tmp = y1 % 256;
    y1 = tmp;
    tmp = x2 % 256;
    x2 = tmp;
    tmp = y2 % 256;
    y2 = tmp;
    if (x < 0)
        x = 0;
    if (x > 127)
        x = 127;
    if (y < 0)
        y = 0;
    if (y > 127)
        y = 127;
    if (x1 < 0)
        x1 = 0;
    if (x1 > 127)
        x1 = 127;
    if (y1 < 0)
        y1 = 0;
    if (y1 > 127)
        y1 = 127;
    if (x2 < 0)
        x2 = 0;
    if (x2 > 127)
        x2 = 127;
    if (y2 < 0)
        y2 = 0;
    if (y2 > 127)
        y2 = 127;
    if (x2 < x1)
    {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y2 < y1)
    {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    // just check if coords inside region
    return ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2));
}

bool check_unit_dead(int *p)
{
    bool dead = false;
    if (p)
    {
        if ((*((byte *)((uintptr_t)p + S_FLAGS3)) & (SF_DEAD | SF_DIEING | SF_UNIT_FREE)) != 0)
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_unit_complete(int *p) // for buildings
{
    bool f = false;
    if (p)
    {
        if ((*((byte *)((uintptr_t)p + S_FLAGS3)) & SF_COMPLETED) != 0) // flags3 last bit
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_hidden(int *p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte *)((uintptr_t)p + S_FLAGS3)) & SF_HIDDEN) != 0) // flags3 4 bit
            f = true;
    }
    else
        f = true;
    return f;
}

bool check_unit_preplaced(int *p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte *)((uintptr_t)p + S_FLAGS3)) & SF_PREPLACED) != 0) // flags3
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_near_death(int *p)
{
    bool dead = false;
    if (p)
    {
        if (((*((byte *)((uintptr_t)p + S_FLAGS3)) & SF_DIEING) != 0) && ((*((byte *)((uintptr_t)p + S_FLAGS3)) & (SF_DEAD | SF_UNIT_FREE)) == 0))
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_peon_loaded(int *p, byte r)
{
    bool f = false;
    if (p)
    {
        if (r == 0)
        {
            if (((*((byte *)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0) && ((*((byte *)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_GOLD) != 0))
                f = true;
        }
        if (r == 1)
        {
            if (((*((byte *)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0) && ((*((byte *)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_LUMBER) != 0))
                f = true;
        }
        if (r == 2)
        {
            if (((*((byte *)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0))
                f = true;
        }
    }
    return f;
}

void find_all_units(byte id)
{
    // CAREFUL with this function - ALL units get into massive
    // even if their memory was cleared already
    // all units by id will go in array
    units = 0;
    int *p = (int *)UNITS_MASSIVE; // pointer to units
    p = (int *)(*p);
    int k = *(int *)UNITS_NUMBER;
    while (k > 0)
    {
        bool f = *((byte *)((uintptr_t)p + S_ID)) == (byte)id;
        if (f)
        {
            unit[units] = p;
            units++;
        }
        p = (int *)((int)p + 0x98);
        k--;
    }
}

void find_all_alive_units(byte id)
{
    // all units by id will go in array
    units = 0;
    for (int i = 0; i < 16; i++)
    {
        int *p = (int *)(UNITS_LISTS + 4 * i); // pointer to units list for each player
        if (p)
        {
            p = (int *)(*p);
            while (p)
            {
                bool f = *((byte *)((uintptr_t)p + S_ID)) == (byte)id;
                if (id == ANY_BUILDING)
                    f = *((byte *)((uintptr_t)p + S_ID)) >= U_FARM; // buildings
                if (id == ANY_MEN)
                    f = *((byte *)((uintptr_t)p + S_ID)) < U_FARM; // all nonbuildings
                if (id == ANY_UNITS)
                    f = true;               // all ALL units
                if (id == ANY_BUILDING_2x2) // small buildings
                {
                    byte sz = *((byte *)UNIT_SIZE_TABLE + *((byte *)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 2;
                }
                if (id == ANY_BUILDING_3x3) // med buildings
                {
                    byte sz = *((byte *)UNIT_SIZE_TABLE + *((byte *)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 3;
                }
                if (id == ANY_BUILDING_4x4) // big buildings
                {
                    byte sz = *((byte *)UNIT_SIZE_TABLE + *((byte *)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 4;
                }
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        unit[units] = p;
                        units++;
                    }
                }
                p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void sort_complete()
{
    // only completed units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_complete(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_in_region()
{
    // only units in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD *)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD *)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_not_in_region()
{
    // only units not in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD *)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD *)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (!in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_target_in_region()
{
    // only units that have order coords in region stay in array
    int k = 0;
    byte x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((byte *)((uintptr_t)unit[i] + S_ORDER_X));
        y = *((byte *)((uintptr_t)unit[i] + S_ORDER_Y));
        if (in_region(x, y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_stat(byte pr, int v, byte cmp)
{
    // only units stay in array if have property compared to value is true
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (cmp_stat(unit[i], v, pr, cmp))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_hidden()
{
    // only not hidden units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_hidden(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_self(int *u)
{
    // unit remove self from array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!(unit[i] == u))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_full_hp()
{
    // if hp not full
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte id = *((byte *)((uintptr_t)unit[i] + S_ID)); // unit id
        WORD mhp = *(WORD *)(UNIT_HP_TABLE + 2 * id);     // max hp
        WORD hp = *((WORD *)((uintptr_t)unit[i] + S_HP)); // unit hp
        if (hp < mhp)                                     // hp not full
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_fleshy()
{
    // only fleshy units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte id = *((byte *)((uintptr_t)unit[i] + S_ID));            // unit id
        if ((*(int *)(UNIT_GLOBAL_FLAGS + id * 4) & IS_FLESHY) != 0) // fleshy global flag
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_order_hp()
{
    // order array by hp from low to high
    for (int i = 0; i < units; i++)
    {
        int sm = i;
        for (int j = i + 1; j < units; j++)
        {
            WORD hpsm = *((WORD *)((uintptr_t)unit[sm] + S_HP)); // unit hp
            WORD hpj = *((WORD *)((uintptr_t)unit[j] + S_HP));   // unit hp
            if (hpj < hpsm)
            {
                sm = j;
            }
        }
        int *tmp = unit[i];
        unit[i] = unit[sm];
        unit[sm] = tmp;
    }
}

void sort_preplaced()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_preplaced(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_near_death()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_near_death(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit(int *p)
{
    // only units stay in array that *p can attack them
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int (*)(int *, int *))F_ATTACK_CAN_HIT)(p, unit[i]); // attack can hit original war2 function
        if (a != 0)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit_range(int *p)
{
    // only units stay in array that *p can attack them and have passable terrain in attack range
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int (*)(int *, int *))F_ATTACK_CAN_HIT)(p, unit[i]); // attack can hit
        if (a != 0)
        {
            byte id = *((byte *)((uintptr_t)unit[i] + S_ID));
            byte szx = *(byte *)(UNIT_SIZE_TABLE + 4 * id);
            byte szy = *(byte *)(UNIT_SIZE_TABLE + 4 * id + 2);
            byte idd = *((byte *)((uintptr_t)p + S_ID));
            byte rng = *(byte *)(UNIT_RANGE_TABLE + idd);
            byte ms = *(byte *)MAP_SIZE;
            byte xx = *((byte *)((uintptr_t)unit[i] + S_X));
            byte yy = *((byte *)((uintptr_t)unit[i] + S_Y));
            if (xx < rng)
                xx = 0;
            else
                xx -= rng;
            if (yy < rng)
                yy = 0;
            else
                yy -= rng;
            byte cl = *((byte *)((uintptr_t)p + S_MOVEMENT_TYPE));       // movement type
            WORD mt = *(WORD *)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl); // movement terrain flags

            bool f = false;
            for (int x = xx; (x < szx + xx + rng * 2 + 1) && (x < 127); x++)
            {
                for (int y = yy; (y < szy + yy + rng * 2 + 1) && (x < 127); y++)
                {
                    int aa = 1;
                    if ((cl == 0) || (cl == 3)) // land and docked transport
                    {
                        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt); // original war2 func if terrain passable with that movement type
                    }
                    if ((x % 2 == 0) && (y % 2 == 0)) // air and water
                    {
                        if ((cl == 1) || (cl == 2))
                        {
                            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
                        }
                    }
                    if (aa == 0)
                        f = true;
                }
            }
            if (f)
            {
                unitt[k] = unit[i];
                k++;
            }
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_rune_near()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte *)((uintptr_t)unit[i] + S_X));
        byte y = *((byte *)((uintptr_t)unit[i] + S_Y));
        bool f = false;
        for (int r = 0; r < 50; r++) // max runes 50
        {
            WORD d = *(WORD *)(RUNEMAP_TIMERS + 2 * r);
            if (d != 0)
            {
                byte xx = *(byte *)(RUNEMAP_X + r);
                byte yy = *(byte *)(RUNEMAP_Y + r);
                if (xx == x)
                {
                    if (yy > y)
                    {
                        if ((yy - y) == 1)
                            f = true;
                    }
                    else
                    {
                        if ((y - yy) == 1)
                            f = true;
                    }
                }
                if (yy == y)
                {
                    if (xx > x)
                    {
                        if ((xx - x) == 1)
                            f = true;
                    }
                    else
                    {
                        if ((x - xx) == 1)
                            f = true;
                    }
                }
            }
        }
        if (!f)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_not_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void set_stat_all(byte pr, int v)
{
    for (int i = 0; i < units; i++)
    {
        set_stat(unit[i], v, pr); // set stat to all units in array
    }
}

void kill_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_kill(unit[i]); // just kill all in array
    }
    units = 0;
}

void remove_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_remove(unit[i]); // just kill all in array
    }
    units = 0;
}

void cast_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_cast(unit[i]); // casting spells
    }
    units = 0;
}

void damag(int *p, byte n1, byte n2)
{
    WORD hp = *((WORD *)((uintptr_t)p + S_HP)); // unit hp
    WORD n = n1 + 256 * n2;
    if (hp > n)
    {
        hp -= n;
        set_stat(p, hp, S_HP);
    }
    else
    {
        set_stat(p, 0, S_HP);
        unit_kill(p);
    }
}

void damag_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        damag(unit[i], n1, n2);
    }
}

void heal(int *p, byte n1, byte n2)
{
    byte id = *((byte *)((uintptr_t)p + S_ID));   // unit id
    WORD mhp = *(WORD *)(UNIT_HP_TABLE + 2 * id); // max hp
    WORD hp = *((WORD *)((uintptr_t)p + S_HP));   // unit hp
    WORD n = n1 + 256 * n2;
    if (hp < mhp)
    {
        hp += n;
        if (hp > mhp)
            hp = mhp; // canot heal more than max hp
        set_stat(p, hp, S_HP);
    }
}

void heal_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        heal(unit[i], n1, n2);
    }
}

void peon_load(int *u, byte r)
{
    byte f = *((byte *)((uintptr_t)u + S_PEON_FLAGS));
    if (!(f & PEON_LOADED))
    {
        if (r == 0)
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_GOLD;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int *))F_GROUP_SET)(u);
        }
        else
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_LUMBER;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int *))F_GROUP_SET)(u);
        }
    }
}

void peon_load_all(byte r)
{
    for (int i = 0; i < units; i++)
    {
        peon_load(unit[i], r);
    }
}

void viz_area(byte x, byte y, byte pl, byte sz)
{
    int Vf = F_VISION2;
    switch (sz)
    {
    case 0:
        Vf = F_VISION2;
        break;
    case 1:
        Vf = F_VISION2;
        break;
    case 2:
        Vf = F_VISION2;
        break;
    case 3:
        Vf = F_VISION3;
        break;
    case 4:
        Vf = F_VISION4;
        break;
    case 5:
        Vf = F_VISION5;
        break;
    case 6:
        Vf = F_VISION6;
        break;
    case 7:
        Vf = F_VISION7;
        break;
    case 8:
        Vf = F_VISION8;
        break;
    case 9:
        Vf = F_VISION9;
        break;
    default:
        Vf = F_VISION2;
        break;
    }
    for (byte i = 0; i < 8; i++)
    {
        if (((1 << i) & pl) != 0)
        {
            ((void (*)(WORD, WORD, byte))Vf)(x, y, i);
        }
    }
}

void viz_area_add(byte x, byte y, byte pl, byte sz)
{
    if ((vizs_n >= 0) && (vizs_n <= 255))
    {
        vizs_areas[vizs_n].x = x;
        vizs_areas[vizs_n].y = y;
        vizs_areas[vizs_n].p = pl;
        vizs_areas[vizs_n].s = sz;
        vizs_n++;
    }
}

void viz_area_all(byte pl, byte sz)
{
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte *)((uintptr_t)unit[i] + S_X));
        byte y = *((byte *)((uintptr_t)unit[i] + S_Y));
        viz_area_add(x, y, pl, sz);
    }
}

void give(int *p, byte owner)
{
    ((void (*)(int *, byte, byte))F_CAPTURE)(p, owner, 1); // original capture unit war2 func
    *(byte *)(RESCUED_UNITS + 2 * owner) -= 1;             // reset number of captured units
}

void give_all(byte o)
{
    for (int i = 0; i < units; i++)
    {
        give(unit[i], o);
    }
}

bool unit_move(byte x, byte y, int *unit)
{
    if (x < 0)
        return false;
    if (y < 0)
        return false;             // canot go negative
    byte mxs = *(byte *)MAP_SIZE; // map size
    if (x >= mxs)
        return false;
    if (y >= mxs)
        return false; // canot go outside map
    if (check_unit_hidden(unit))
        return false;                                            // if unit not hidden
    byte cl = *((byte *)((uintptr_t)unit + S_MOVEMENT_TYPE));    // movement type
    WORD mt = *(WORD *)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl); // movement terrain flags

    int aa = 1;
    if ((cl == 0) || (cl == 3)) // land and docked transport
    {
        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt); // original war2 func if terrain passable with that movement type
    }
    if ((x % 2 == 0) && (y % 2 == 0)) // air and water
    {
        if ((cl == 1) || (cl == 2))
        {
            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
        }
    }
    if (aa == 0)
    {
        ((void (*)(int *))F_UNIT_UNPLACE)(unit); // unplace
        set_stat(unit, x, S_X);
        set_stat(unit, y, S_Y); // change real coords
        set_stat(unit, x * 32, S_DRAW_X);
        set_stat(unit, y * 32, S_DRAW_Y);      // change draw sprite coords
        ((void (*)(int *))F_UNIT_PLACE)(unit); // place
        return true;
    }
    return false;
}

void move_all(byte x, byte y)
{
    sort_stat(S_ID, U_FARM, CMP_SMALLER); // non buildings
    sort_stat(S_ANIMATION, 2, CMP_EQ);    // only if animation stop
    for (int i = 0; i < units; i++)
    {
        int xx = 0, yy = 0, k = 1;
        bool f = unit_move(x, y, unit[i]);
        xx--;
        while ((!f) & (k < 5)) // goes in spiral like original war2 (size 5)
        {
            while ((!f) & (yy < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy++;
            }
            while ((!f) & (xx < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx++;
            }
            while ((!f) & (yy > -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy--;
            }
            while ((!f) & (xx >= -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx--;
            }
            k++;
        }
    }
}

void give_order(int *u, byte x, byte y, byte o)
{
    byte id = *((byte *)((uintptr_t)u + S_ID));
    if (id < U_FARM)
    {
        char buf[] = "\x0";
        bool f = ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT));
        if (f)
        {
            buf[0] = o;
            PATCH_SET((char *)GW_ACTION_TYPE, buf);
        }
        int *tr = NULL;
        for (int i = 0; i < 16; i++)
        {
            int *p = (int *)(UNITS_LISTS + 4 * i); // pointer to units list for each player
            if (p)
            {
                p = (int *)(*p);
                while (p)
                {
                    if (p != u)
                    {
                        if (!check_unit_dead(p) && !check_unit_hidden(p))
                        {
                            byte xx = *(byte *)((uintptr_t)p + S_X);
                            byte yy = *(byte *)((uintptr_t)p + S_Y);
                            if ((abs(x - xx) <= 2) && (abs(y - yy) <= 2))
                            {
                                if (f)
                                {
                                    byte idd = *(byte *)((uintptr_t)p + S_ID);
                                    if (idd < U_FARM)
                                    {
                                        bool trf = true;
                                        if (o == ORDER_SPELL_ARMOR)
                                        {
                                            WORD ef = *(WORD *)((uintptr_t)p + S_SHIELD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_BLOODLUST)
                                        {
                                            WORD ef = *(WORD *)((uintptr_t)p + S_BLOOD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_HASTE)
                                        {
                                            WORD ef = *(WORD *)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef > 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_SLOW)
                                        {
                                            WORD ef = *(WORD *)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef <= 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_POLYMORPH)
                                        {
                                            trf = idd != U_CRITTER;
                                        }
                                        if (o == ORDER_SPELL_HEAL)
                                        {
                                            WORD mhp = *(WORD *)(UNIT_HP_TABLE + 2 * idd);
                                            WORD hp = *((WORD *)((uintptr_t)p + S_HP));
                                            trf = hp < mhp;
                                        }
                                        if (trf)
                                        {
                                            WORD efi = *(WORD *)((uintptr_t)p + S_INVIZ);
                                            trf = efi == 0;
                                        }
                                        if (trf)
                                            tr = p;
                                    }
                                }
                                else
                                    tr = p;
                            }
                        }
                    }
                    p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
        bool aoe = (o == ORDER_SPELL_VISION) || (o == ORDER_SPELL_EXORCISM) || (o == ORDER_SPELL_FIREBALL) ||
                   (o == ORDER_SPELL_BLIZZARD) || (o == ORDER_SPELL_EYE) || (o == ORDER_SPELL_RAISEDEAD) ||
                   (o == ORDER_SPELL_DRAINLIFE) || (o == ORDER_SPELL_WHIRLWIND) || (o == ORDER_SPELL_RUNES) ||
                   (o == ORDER_SPELL_ROT) || (o == ORDER_MOVE) || (o == ORDER_PATROL) ||
                   (o == ORDER_ATTACK_AREA) || (o == ORDER_ATTACK_WALL) || (o == ORDER_STAND) ||
                   (o == ORDER_ATTACK_GROUND) || (o == ORDER_ATTACK_GROUND_MOVE) || (o == ORDER_DEMOLISH) ||
                   (o == ORDER_HARVEST) || (o == ORDER_RETURN) || (o == ORDER_UNLOAD_ALL) || (o == ORDER_STOP);

        if (o != ORDER_ATTACK_WALL)
        {
            int ord = *(int *)(ORDER_FUNCTIONS + 4 * o); // orders functions
            if (!aoe && (tr != NULL) && (tr != u))
                ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(u, 0, 0, tr, ord); // original war2 order
            if (aoe)
                ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(u, x, y, NULL, ord); // original war2 order
        }
        else
        {
            byte oru = *(byte *)((uintptr_t)u + S_ORDER);
            if (oru != ORDER_ATTACK_WALL)
            {
                int ord = *(int *)(ORDER_FUNCTIONS + 4 * ORDER_STOP);                      // orders functions
                ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(u, 0, 0, NULL, ord); // original war2 order
            }
            set_stat(u, ORDER_ATTACK_WALL, S_NEXT_ORDER);
            set_stat(u, x, S_ORDER_X);
            set_stat(u, y, S_ORDER_Y);
        }

        if (f)
        {
            buf[0] = 0;
            PATCH_SET((char *)GW_ACTION_TYPE, buf);
        }
    }
}

void give_order_spell_target(int *u, int *t, byte o)
{
    if ((u != NULL) && (t != NULL))
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if (id < U_FARM)
        {
            char buf[] = "\x0";
            if ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT))
            {
                buf[0] = o;
                PATCH_SET((char *)GW_ACTION_TYPE, buf);

                int ord = *(int *)(ORDER_FUNCTIONS + 4 * o);                            // orders functions
                ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(u, 0, 0, t, ord); // original war2 order

                buf[0] = 0;
                PATCH_SET((char *)GW_ACTION_TYPE, buf);
            }
        }
    }
}

void give_order_target(int* u, byte x, byte y, int* tr, byte o)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id < U_FARM)
    {
        char buf[] = "\x0";
        bool f = ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT));
        if (f)
        {
            buf[0] = o;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }

        bool aoe = (o == ORDER_SPELL_VISION) || (o == ORDER_SPELL_EXORCISM) || (o == ORDER_SPELL_FIREBALL) ||
            (o == ORDER_SPELL_BLIZZARD) || (o == ORDER_SPELL_EYE) || (o == ORDER_SPELL_RAISEDEAD) ||
            (o == ORDER_SPELL_DRAINLIFE) || (o == ORDER_SPELL_WHIRLWIND) || (o == ORDER_SPELL_RUNES) ||
            (o == ORDER_SPELL_ROT) || (o == ORDER_MOVE) || (o == ORDER_PATROL) ||
            (o == ORDER_ATTACK_AREA) || (o == ORDER_ATTACK_WALL) || (o == ORDER_STAND) ||
            (o == ORDER_ATTACK_GROUND) || (o == ORDER_ATTACK_GROUND_MOVE) || (o == ORDER_DEMOLISH) || (o == ORDER_DEMOLISH_AT) ||
            (o == ORDER_HARVEST) || (o == ORDER_RETURN) || (o == ORDER_UNLOAD_ALL) || (o == ORDER_STOP);

        if ((o == ORDER_DEMOLISH_AT) || (o == ORDER_DEMOLISH_NEAR))o = ORDER_DEMOLISH;

        if (o > ORDER_NONE)//peon create building
        {
            byte bid = o - ORDER_NONE - 1;

            set_stat(u, bid, S_PEON_BUILD);
            set_stat(u, x, S_PEON_BUILD_X);
            set_stat(u, y, S_PEON_BUILD_Y);

            int ord = 0x00436A80;//do_unit_create_bldg
            ((void (*)(int*, DWORD, byte))0x0041F670)(u, (DWORD)((uintptr_t)u + S_ORDER_X), bid);//original war2 find_bldg_entry_corner(peon,&buildXY,building_id)
            ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, x, y, NULL, ord);//original war2 order
        }

        if (o != ORDER_ATTACK_WALL)
        {
            int ord = *(int*)(ORDER_FUNCTIONS + 4 * o);//orders functions
            if (!aoe && (tr != NULL) && (tr != u))
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, tr, ord);//original war2 order
            if (aoe)
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, x, y, NULL, ord);//original war2 order
        }
        else
        {
            byte oru = *(byte*)((uintptr_t)u + S_ORDER);
            if (oru != ORDER_ATTACK_WALL)
            {
                int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_STOP);//orders functions
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, NULL, ord);//original war2 order
            }
            set_stat(u, ORDER_ATTACK_WALL, S_NEXT_ORDER);
            set_stat(u, x, S_ORDER_X);
            set_stat(u, y, S_ORDER_Y);
        }

        if (f)
        {
            buf[0] = 0;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }
    }
}

void order_all(byte x, byte y, byte o)
{
    for (int i = 0; i < units; i++)
    {
        give_order(unit[i], x, y, o);
    }
}

bool check_ally(byte p1, byte p2)
{
    // check allied table
    return ((*(byte *)(ALLY + p1 + 16 * p2) != 0) && (*(byte *)(ALLY + p2 + 16 * p1) != 0));
}

bool slot_alive(byte p)
{
    return (get_val(ALL_BUILDINGS, p) + (get_val(ALL_UNITS, p) - get_val(FLYER, p))) > 0; // no units and buildings
}

void ally(byte p1, byte p2, byte a)
{
    // set ally bytes in table
    *(byte *)(ALLY + p1 + 16 * p2) = a;
    *(byte *)(ALLY + p2 + 16 * p1) = a;
    ((void (*)())F_RESET_COLORS)(); // orig war2 func reset colors of sqares around units
}

void ally_one_sided(byte p1, byte p2, byte a)
{
    // set ally bytes in table
    *(byte *)(ALLY + p1 + 16 * p2) = a;
    ((void (*)())F_RESET_COLORS)(); // orig war2 func reset colors of sqares around units
}

bool check_opponents(byte player)
{
    // check if player have opponents
    bool f = false;
    byte o = C_NOBODY;
    for (byte i = 0; i < 8; i++)
    {
        if (player != i)
        {
            if (slot_alive(i) && !check_ally(player, i)) // if enemy and not dead
                f = true;
        }
    }
    return f;
}

void viz(int p1, int p2, byte a)
{
    // set vision bits
    byte v = *(byte *)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte *)(VIZ + p1) = v;

    v = *(byte *)(VIZ + p2);
    if (a == 0)
        v = v & (~(1 << p1));
    else
        v = v | (1 << p1);
    *(byte *)(VIZ + p2) = v;
}

void viz_one_sided(int p1, int p2, byte a)
{
    // set vision bits
    byte v = *(byte *)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte *)(VIZ + p1) = v;
}

void comps_vision(bool v)
{
    // comps can give vision too
    if (v)
    {
        char o[] = "\x0";
        PATCH_SET((char *)COMPS_VIZION, o);
        char o2[] = "\x90\x90";
        PATCH_SET((char *)COMPS_VIZION2, o2);
        char o3[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char *)COMPS_VIZION3, o3);
    }
    else
    {
        char o[] = "\xAA";
        PATCH_SET((char *)COMPS_VIZION, o);
        char o2[] = "\x84\xC9";
        PATCH_SET((char *)COMPS_VIZION2, o2);
        char o3[] = "\xF\x85\x8C\x0\x0\x0";
        PATCH_SET((char *)COMPS_VIZION3, o3);
    }
}

void change_res(byte p, byte r, byte k, int m)
{
    int a = GOLD;
    int *rs = (int *)a;
    DWORD res = 0;
    bool s = false;
    if (p >= 0 && p <= 8) // player id
    {
        switch (r) // select resource and add or substract it
        {
        case 0:
            a = GOLD + 4 * p;
            s = false;
            break;
        case 1:
            a = LUMBER + 4 * p;
            s = false;
            break;
        case 2:
            a = OIL + 4 * p;
            s = false;
            break;
        case 3:
            a = GOLD + 4 * p;
            s = true;
            break;
        case 4:
            a = LUMBER + 4 * p;
            s = true;
            break;
        case 5:
            a = OIL + 4 * p;
            s = true;
            break;
        default:
            break;
        }
        if (r >= 0 && r <= 5)
        {
            rs = (int *)a; // resourse pointer
            if (s)
            {
                if (*rs > (int)(k * m))
                    res = *rs - (k * m);
                else
                    res = 0; // canot go smaller than 0
            }
            else
            {
                if (*rs <= (256 * 256 * 256 * 32))
                    res = *rs + (k * m);
            }
            patch_setdword((DWORD *)a, res);
        }
    }
}

void add_total_res(byte p, byte r, byte k, int m)
{
    int a = GOLD_TOTAL;
    int *rs = (int *)a;
    DWORD res = 0;
    if (p >= 0 && p <= 8) // player id
    {
        switch (r) // select resource and add or substract it
        {
        case 0:
            a = GOLD_TOTAL + 4 * p;
            break;
        case 1:
            a = LUMBER_TOTAL + 4 * p;
            break;
        case 2:
            a = OIL_TOTAL + 4 * p;
            break;
        default:
            break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int *)a; // resourse pointer
            if (*rs <= (256 * 256 * 256 * 32))
                res = *rs + (k * m);
            patch_setdword((DWORD *)a, res);
        }
    }
}

void set_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4)
{
    // as before but dnt add or sub res, just set given value
    char buf[4] = {0};
    int a = 0;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:
            break;
        }
        if (r >= 0 && r <= 2)
        {
            buf[0] = k1;
            buf[1] = k2;
            buf[2] = k3;
            buf[3] = k4;
            PATCH_SET((char *)a, buf);
        }
    }
}

bool cmp_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4, byte cmp)
{
    // compare resource to value
    int a = GOLD;
    int *rs = (int *)a;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:
            break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int *)a;
            return cmp_args4(cmp, *rs, k1 + 256 * k2 + 256 * 256 * k3 + 256 * 256 * 256 * k4);
        }
    }
    return false;
}

int empty_false(byte) { return 0; } // always return false function
int empty_true(byte) { return 1; }  // always return true function
void empty_build(int id)
{
    ((void (*)(int))F_TRAIN_UNIT)(id); // original build unit func
}
void empty_build_building(int id)
{
    ((void (*)(int))F_BUILD_BUILDING)(id); // original build func
}
void empty_build_research(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH)(id);
}
void empty_build_research_spell(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH_SPELL)(id);
}
void empty_build_upgrade_self(int id)
{
    ((void (*)(int))F_BUILD_UPGRADE_SELF)(id);
}
void empty_cast_spell(int id)
{
    ((void (*)(int))F_CAST_SPELL)(id);
}

void empty_upgrade_th1(int id)
{
    int *u = (int *)*(int *)LOCAL_UNITS_SELECTED;
    set_stat(u, 0, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}
void empty_upgrade_th2(int id)
{
    int *u = (int *)*(int *)LOCAL_UNITS_SELECTED;
    set_stat(u, 1, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}

int empty_research_swords(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SWORDS)(id); }       // 0 or 1
int empty_research_shield(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIELD)(id); }       // 0 or 1
int empty_research_cat(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_CAT)(id); }             // 0 or 1
int empty_research_arrows(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_ARROWS)(id); }       // 0 or 1
int empty_research_ships_at(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_AT)(id); }   // 0 or 1
int empty_research_ships_def(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_DEF)(id); } // 0 or 1
int empty_research_ranger(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_RANGER)(id); }
int empty_research_scout(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SCOUT)(id); }
int empty_research_long(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_LONG)(id); }
int empty_research_marks(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_MARKS)(id); }
int empty_research_spells(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SPELL)(id); }
// 00444410
int empty_upgrade_th(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TH)(id); }       // 0 or 1
int empty_upgrade_tower(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TOWER)(id); } // 0 or 1
int empty_spell_learned(byte id) { return ((int (*)(int))F_CHECK_SPELL_LEARNED)(id); }

int _2tir()
{
    if ((get_val(TH2, *(byte *)LOCAL_PLAYER) != 0) || (get_val(TH3, *(byte *)LOCAL_PLAYER) != 0))
        return 1;
    else
        return 0;
}

int _3tir()
{
    if (get_val(TH3, *(byte *)LOCAL_PLAYER) != 0)
        return 1;
    else
        return 0;
}

int get_marks()
{
    if (*(byte *)(GB_MARKS + *(byte *)LOCAL_PLAYER))
        return 1;
    else
        return 0;
}

void repair_cat(bool b)
{
    // peon can repair unit if it have transport flag OR catapult flag
    if (b)
    {
        char r1[] = "\xeb\x75\x90\x90\x90"; // f6 c4 04 74 14
        PATCH_SET((char *)REPAIR_FLAG_CHECK2, r1);
        char r2[] = "\x66\xa9\x04\x04\x74\x9c\xeb\x86";
        PATCH_SET((char *)REPAIR_CODE_CAVE, r2);
    }
    else
    {
        char r1[] = "\xf6\xc4\x4\x74\x14";
        PATCH_SET((char *)REPAIR_FLAG_CHECK2, r1);
    }
}

void fireball_dmg(byte dmg)
{
    char fb[] = "\x28"; // 40 default
    fb[0] = dmg;
    PATCH_SET((char *)FIREBALL_DMG, fb);
}

void trigger_time(byte tm)
{
    // war2 will call victory check function every 200 game ticks
    char ttime[] = "\xc8"; // 200 default
    ttime[0] = tm;
    PATCH_SET((char *)TRIG_TIME, ttime);
}

void manacost(byte id, byte c)
{
    // spells cost of mana
    char mana[] = "\x1";
    mana[0] = c;
    PATCH_SET((char *)(MANACOST + 2 * id), mana);
}

void upgr(byte id, byte c)
{
    // upgrades power
    char up[] = "\x1";
    up[0] = c;
    PATCH_SET((char *)(UPGRD + id), up);
}

byte get_upgrade(byte id, byte pl)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:
        a = GB_ARROWS;
        break;
    case SWORDS:
        a = GB_SWORDS;
        break;
    case ARMOR:
        a = GB_SHIELDS;
        break;
    case SHIP_DMG:
        a = GB_BOAT_ATTACK;
        break;
    case SHIP_ARMOR:
        a = GB_BOAT_ARMOR;
        break;
    case SHIP_SPEED:
        a = GB_BOAT_SPEED;
        break;
    case CATA_DMG:
        a = GB_CAT_DMG;
        break;
    default:
        a = GB_ARROWS;
        break;
    }
    return *(byte *)(a + pl);
}

void set_upgrade(byte id, byte pl, byte v)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:
        a = GB_ARROWS;
        break;
    case SWORDS:
        a = GB_SWORDS;
        break;
    case ARMOR:
        a = GB_SHIELDS;
        break;
    case SHIP_DMG:
        a = GB_BOAT_ATTACK;
        break;
    case SHIP_ARMOR:
        a = GB_BOAT_ARMOR;
        break;
    case SHIP_SPEED:
        a = GB_BOAT_SPEED;
        break;
    case CATA_DMG:
        a = GB_CAT_DMG;
        break;
    default:
        a = GB_ARROWS;
        break;
    }
    char buf[] = "\x0";
    buf[0] = v;
    PATCH_SET((char *)(a + pl), buf);
    ((void (*)())F_STATUS_REDRAW)(); // status redraw
}

void center_view(byte x, byte y)
{
    ((void (*)(byte, byte))F_MINIMAP_CLICK)(x, y); // original war2 func that called when player click on minimap
}

PROC g_proc_00451054;
void count_add_to_tables_load_game(int *u)
{
    if (saveload_fixed)
    {
        byte f = *((byte *)((uintptr_t)u + S_AI_AIFLAGS));
        byte ff = f | AI_PASSIVE;
        set_stat(u, ff, S_AI_AIFLAGS);
        ((void (*)(int *))g_proc_00451054)(u); // original
        set_stat(u, f, S_AI_AIFLAGS);
    }
    else
        ((void (*)(int *))g_proc_00451054)(u); // original
}

PROC g_proc_00438A5C;
PROC g_proc_00438985;
void unset_peon_ai_flags(int *u)
{
    ((void (*)(int *))g_proc_00438A5C)(u); // original
    if (saveload_fixed)
    {
        char rep[] = "\x0\x0";
        WORD p = 0;
        for (int i = 0; i < 8; i++)
        {
            p = *((WORD *)((uintptr_t)SGW_REPAIR_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char *)(SGW_REPAIR_PEONS + 2 * i), rep);
            p = *((WORD *)((uintptr_t)SGW_GOLD_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char *)(SGW_GOLD_PEONS + 2 * i), rep);
            p = *((WORD *)((uintptr_t)SGW_TREE_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char *)(SGW_TREE_PEONS + 2 * i), rep);
        }
    }
}

void tech_built(int p, byte t)
{
    ((void (*)(int, byte))F_TECH_BUILT)(p, t);
}

void tech_reinit()
{
    for (int i = 0; i < 8; i++)
    {
        byte o = *(byte *)(CONTROLER_TYPE + i);
        byte a = 0;
        int s = 0;
        if (o == C_COMP)
        {
            a = *(byte *)(GB_ARROWS + i);
            if (a > 0)
                tech_built(i, UP_ARROW1);
            if (a > 1)
                tech_built(i, UP_ARROW2);
            a = *(byte *)(GB_SWORDS + i);
            if (a > 0)
                tech_built(i, UP_SWORD1);
            if (a > 1)
                tech_built(i, UP_SWORD2);
            a = *(byte *)(GB_SHIELDS + i);
            if (a > 0)
                tech_built(i, UP_SHIELD1);
            if (a > 1)
                tech_built(i, UP_SHIELD2);
            a = *(byte *)(GB_BOAT_ATTACK + i);
            if (a > 0)
                tech_built(i, UP_BOATATK1);
            if (a > 1)
                tech_built(i, UP_BOATATK2);
            a = *(byte *)(GB_BOAT_ARMOR + i);
            if (a > 0)
                tech_built(i, UP_BOATARM1);
            if (a > 1)
                tech_built(i, UP_BOATARM2);
            a = *(byte *)(GB_CAT_DMG + i);
            if (a > 0)
                tech_built(i, UP_CATDMG1);
            if (a > 1)
                tech_built(i, UP_CATDMG2);
            a = *(byte *)(GB_RANGER + i);
            if (a)
                tech_built(i, UP_RANGER);
            a = *(byte *)(GB_MARKS + i);
            if (a)
                tech_built(i, UP_SKILL1);
            a = *(byte *)(GB_LONGBOW + i);
            if (a)
                tech_built(i, UP_SKILL2);
            a = *(byte *)(GB_SCOUTING + i);
            if (a)
                tech_built(i, UP_SKILL3);

            s = *(int *)(SPELLS_LEARNED + 4 * i);
            if (s & (1 << L_ALTAR_UPGR))
                tech_built(i, UP_CLERIC);
            if (s & (1 << L_HEAL))
                tech_built(i, UP_CLERIC1);
            if (s & (1 << L_BLOOD))
                tech_built(i, UP_CLERIC1);
            if (s & (1 << L_EXORCISM))
                tech_built(i, UP_CLERIC2);
            if (s & (1 << L_RUNES))
                tech_built(i, UP_CLERIC2);
            if (s & (1 << L_FLAME_SHIELD))
                tech_built(i, UP_WIZARD1);
            if (s & (1 << L_RAISE))
                tech_built(i, UP_WIZARD1);
            if (s & (1 << L_SLOW))
                tech_built(i, UP_WIZARD2);
            if (s & (1 << L_HASTE))
                tech_built(i, UP_WIZARD2);
            if (s & (1 << L_INVIS))
                tech_built(i, UP_WIZARD3);
            if (s & (1 << L_WIND))
                tech_built(i, UP_WIZARD3);
            if (s & (1 << L_POLYMORF))
                tech_built(i, UP_WIZARD4);
            if (s & (1 << L_UNHOLY))
                tech_built(i, UP_WIZARD4);
            if (s & (1 << L_BLIZZARD))
                tech_built(i, UP_WIZARD5);
            if (s & (1 << L_DD))
                tech_built(i, UP_WIZARD5);

            find_all_alive_units(U_KEEP);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
                tech_built(i, UP_KEEP);
            find_all_alive_units(U_STRONGHOLD);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
                tech_built(i, UP_KEEP);
            find_all_alive_units(U_CASTLE);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
            find_all_alive_units(U_FORTRESS);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
        }
    }
}

bool bldg_check_spell_learned(byte id, byte o)
{
    int s = *(int*)(SPELLS_LEARNED + 4 * o);
    if (s & (1 << L_ALTAR_UPGR))if ((id == UG_SP_OGREMAGE) || (id == UG_SP_PALADIN))return true;
    if (s & (1 << L_HEAL))if (id == UG_SP_HEAL)return true;
    if (s & (1 << L_BLOOD))if (id == UG_SP_BLOODLUST)return true;
    if (s & (1 << L_EXORCISM))if (id == UG_SP_EXORCISM)return true;
    if (s & (1 << L_RUNES))if (id == UG_SP_RUNES)return true;
    if (s & (1 << L_FLAME_SHIELD))if (id == UG_SP_FIRESHIELD)return true;
    if (s & (1 << L_RAISE))if (id == UG_SP_RAISEDEAD)return true;
    if (s & (1 << L_SLOW))if (id == UG_SP_SLOW)return true;
    if (s & (1 << L_HASTE))if (id == UG_SP_HASTE)return true;
    if (s & (1 << L_INVIS))if (id == UG_SP_INVIS)return true;
    if (s & (1 << L_WIND))if (id == UG_SP_WHIRLWIND)return true;
    if (s & (1 << L_POLYMORF))if (id == UG_SP_POLYMORPH)return true;
    if (s & (1 << L_UNHOLY))if (id == UG_SP_ARMOR)return true;
    if (s & (1 << L_BLIZZARD))if (id == UG_SP_BLIZZARD)return true;
    if (s & (1 << L_DD))if (id == UG_SP_ROT)return true;
    return false;
}

bool bldg_check_tech_learned(byte id, byte o)
{
    byte a = 0;

    a = *(byte*)(GB_ARROWS + o);
    if (a > 0)if ((id == UG_UPARROW1) || (id == UG_UPSPEAR1))return true;
    if (a > 1)if ((id == UG_UPARROW2) || (id == UG_UPSPEAR2))return true;
    a = *(byte*)(GB_SWORDS + o);
    if (a > 0)if ((id == UG_UPSWORD1) || (id == UG_UPAXE1))return true;
    if (a > 1)if ((id == UG_UPSWORD2) || (id == UG_UPAXE2))return true;
    a = *(byte*)(GB_SHIELDS + o);
    if (a > 0)if ((id == UG_UPSHIELD1) || (id == UG_UPOSHIELD1))return true;
    if (a > 1)if ((id == UG_UPSHIELD2) || (id == UG_UPOSHIELD2))return true;
    a = *(byte*)(GB_BOAT_ATTACK + o);
    if (a > 0)if ((id == UG_UPBOAT_ATTACK1) || (id == UG_UPOBOAT_ATTACK1))return true;
    if (a > 1)if ((id == UG_UPBOAT_ATTACK2) || (id == UG_UPOBOAT_ATTACK2))return true;
    a = *(byte*)(GB_BOAT_ARMOR + o);
    if (a > 0)if ((id == UG_UPBOAT_ARMOR1) || (id == UG_UPOBOAT_ARMOR1))return true;
    if (a > 1)if ((id == UG_UPBOAT_ARMOR2) || (id == UG_UPOBOAT_ARMOR2))return true;
    a = *(byte*)(GB_CAT_DMG + o);
    if (a > 0)if ((id == UG_BALDMG1) || (id == UG_CATDMG1))return true;
    if (a > 1)if ((id == UG_BALDMG2) || (id == UG_CATDMG2))return true;
    a = *(byte*)(GB_RANGER + o);
    if (a)if ((id == UG_UPRANGERS) || (id == UG_UPBERSERKERS))return true;
    a = *(byte*)(GB_MARKS + o);
    if (a)if ((id == UG_UPHMARKS) || (id == UG_UPOMARKS))return true;
    a = *(byte*)(GB_LONGBOW + o);
    if (a)if ((id == UG_UPLONGBOW) || (id == UG_UPLIGHTAXE))return true;
    a = *(byte*)(GB_SCOUTING + o);
    if (a)if ((id == UG_UPHSCOUTS) || (id == UG_UPOSCOUTS))return true;

    return false;
}

int building_start_build(int* u, byte id, byte o)
{
    byte ow = *((byte*)((uintptr_t)u + S_OWNER));

    if (o == 0)
    {
        if ((id == U_KNIGHT) && bldg_check_spell_learned(UG_SP_PALADIN, ow))id = U_PALADIN;
        if ((id == U_OGRE) && bldg_check_spell_learned(UG_SP_OGREMAGE, ow))id = U_OGREMAGE;

        if ((id == U_ARCHER) && bldg_check_tech_learned(UG_UPRANGERS, ow))id = U_RANGER;
        if ((id == U_TROLL) && bldg_check_tech_learned(UG_UPBERSERKERS, ow))id = U_BERSERK;
    }

    return ((int (*)(int*, byte, byte))F_BLDG_START_BUILD)(u, id, o);
}

void build_inventor(int *u)
{
    if (check_unit_complete(u))
    {
        byte f = *((byte *)((uintptr_t)u + S_FLAGS1));
        if (!(f & UF_BUILD_ON))
        {
            byte id = *((byte *)((uintptr_t)u + S_ID));
            byte o = *((byte *)((uintptr_t)u + S_OWNER));
            int spr = get_val(ACTIVE_SAPPERS, o);
            byte nspr = *(byte *)(AIP_NEED_SAP + 48 * o);
            if (nspr > spr)
            {
                if (id == U_INVENTOR)
                    building_start_build(u, U_DWARWES, 0);
                if (id == U_ALCHEMIST)
                    building_start_build(u, U_GOBLINS, 0);
            }
            int flr = get_val(ACTIVE_FLYER, o);
            byte nflr = *(byte *)(AIP_NEED_FLYER + 48 * o);
            if (nflr > flr)
            {
                if (id == U_INVENTOR)
                    building_start_build(u, U_FLYER, 0);
                if (id == U_ALCHEMIST)
                    building_start_build(u, U_ZEPPELIN, 0);
            }
        }
    }
}

void build_sap_fix(bool f)
{
    if (f)
    {
        char b1[] = "\x80\xfa\x40\x0";
        void (*r1)(int *) = build_inventor;
        patch_setdword((DWORD *)b1, (DWORD)r1);
        PATCH_SET((char *)BLDG_WAIT_INVENTOR, b1);       // human inv
        PATCH_SET((char *)(BLDG_WAIT_INVENTOR + 4), b1); // orc inv
    }
    else
    {
        char b1[] = "\x80\xfa\x40\x0";
        PATCH_SET((char *)BLDG_WAIT_INVENTOR, b1);       // human inv
        PATCH_SET((char *)(BLDG_WAIT_INVENTOR + 4), b1); // orc inv
    }
}

void ai_fix_plugin(bool f)
{
    if (f)
    {
        char b1[] = "\xb2\x02";
        PATCH_SET((char *)AIFIX_PEONS_REP, b1); // 2 peon rep
        char b21[] = "\xbb\x8";
        PATCH_SET((char *)AIFIX_GOLD_LUMB1, b21); // gold lumber
        char b22[] = "\xb4\x4";
        PATCH_SET((char *)AIFIX_GOLD_LUMB2, b22); // gold lumber
        char b3[] = "\x1";
        PATCH_SET((char *)AIFIX_BUILD_SIZE, b3); // packed build
        char b4[] = "\xbe\x0\x0\x0\x0\x90\x90";
        PATCH_SET((char *)AIFIX_FIND_HOME, b4); // th corner
        char b6[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char *)AIFIX_POWERBUILD, b6); // powerbuild
        char m7[] = "\x90\x90";
        PATCH_SET((char *)AIFIX_RUNES_INV, m7); // runes no inviz
        ai_fixed = true;
        build_sap_fix(true);
    }
    else
    {
        char b1[] = "\x8a\xd0";
        PATCH_SET((char *)AIFIX_PEONS_REP, b1); // 2 peon rep
        char b21[] = "\xd0\x7";
        PATCH_SET((char *)AIFIX_GOLD_LUMB1, b21); // gold lumber
        char b22[] = "\xf4\x1";
        PATCH_SET((char *)AIFIX_GOLD_LUMB2, b22); // gold lumber
        char b3[] = "\x6";
        PATCH_SET((char *)AIFIX_BUILD_SIZE, b3); // packed build
        char b4[] = "\xe8\xf8\x2a\x1\x0\x8b\xf0";
        PATCH_SET((char *)AIFIX_FIND_HOME, b4); // th corner
        char b6[] = "\xf\x84\x78\x1\x0\x0";
        PATCH_SET((char *)AIFIX_POWERBUILD, b6); // powerbuild
        char m7[] = "\x74\x1d";
        PATCH_SET((char *)AIFIX_RUNES_INV, m7); // runes no inviz
        ai_fixed = false;
        build_sap_fix(false);
    }
}

PROC g_proc_0040EEDD;
void upgrade_tower(int *u, int id, int b)
{
    if (ai_fixed)
    {
        bool c = false;
        byte o = *((byte *)((uintptr_t)u + S_OWNER));
        if ((get_val(LUMBERMILL, o) == 0) && (get_val(SMITH, o) != 0))
            c = true;
        if ((get_val(LUMBERMILL, o) != 0) && (get_val(SMITH, o) != 0) && ((get_val(TOWER, o) % 2) == 0))
            c = true;
        if (c)
            id += 2;
    }
    ((void (*)(int *, int, int))g_proc_0040EEDD)(u, id, b); // original
}

PROC g_proc_00442E25;
void create_skeleton(int x, int y, int id, int o)
{
    if (ai_fixed)
    {
        unit_create((x / 32) + 1, y / 32, id, o % 256, 1);
    }
    else
        ((void (*)(int, int, int, int))g_proc_00442E25)(x, y, id, o); // original
}

PROC g_proc_00425D1C;
int *cast_raise(int *u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte *)((uintptr_t)u + S_OWNER));
        find_all_alive_units(U_SKELETON);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_preplaced();
        if (units < 10)
        {
            if (((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_RAISE)) == 0))
                return NULL;
            byte mp = *((byte *)((uintptr_t)u + S_MANA));
            byte cost = *(byte *)(MANACOST + 2 * RAISE_DEAD);
            if (mp < cost)
                return NULL;
            byte x = *((byte *)((uintptr_t)u + S_X));
            byte y = *((byte *)((uintptr_t)u + S_Y));
            set_region((int)x - 8, (int)y - 8, (int)x + 8, (int)y + 8); // set region around myself
            find_all_units(ANY_BUILDING);                               // dead body
            sort_in_region();
            sort_hidden();
            sort_near_death();
            if (units != 0)
            {
                byte xx = *((byte *)((uintptr_t)unit[0] + S_X));
                byte yy = *((byte *)((uintptr_t)unit[0] + S_Y));
                give_order(u, xx, yy, ORDER_SPELL_RAISEDEAD);
                return unit[0];
            }
        }
        return NULL;
    }
    else
        return ((int *(*)(int *, int, int, int))g_proc_00425D1C)(u, a1, a2, a3); // original
}

PROC g_proc_00424F94;
PROC g_proc_00424FD7;
int *cast_runes(int *u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte *)((uintptr_t)u + S_OWNER));
        if (((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_RUNES)) == 0))
            return NULL;
        byte mp = *((byte *)((uintptr_t)u + S_MANA));
        byte cost = *(byte *)(MANACOST + 2 * RUNES);
        if (mp < cost)
            return NULL;
        byte x = *((byte *)((uintptr_t)u + S_X));
        byte y = *((byte *)((uintptr_t)u + S_Y));
        set_region((int)x - 14, (int)y - 14, (int)x + 14, (int)y + 14); // set region around myself
        find_all_alive_units(ANY_MEN);
        sort_in_region();
        sort_hidden();
        sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
        for (int ui = 0; ui < 16; ui++)
        {
            if (check_ally(o, ui)) // only not allied units
                sort_stat(S_OWNER, ui, CMP_NEQ);
        }
        sort_rune_near();
        if (units != 0)
        {
            byte xx = *((byte *)((uintptr_t)unit[0] + S_X));
            byte yy = *((byte *)((uintptr_t)unit[0] + S_Y));
            give_order(u, xx, yy, ORDER_SPELL_RUNES);
            return unit[0];
        }
        return NULL;
    }
    else
        return ((int *(*)(int *, int, int, int))g_proc_00424F94)(u, a1, a2, a3); // original
}

byte get_manacost(byte s)
{
    return *(byte *)(MANACOST + 2 * s);
}

PROC g_proc_0042757E;
int ai_spell(int *u)
{
    if (ai_fixed)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if ((id == U_MAGE) || (id == U_DK))
        {
            byte x = *((byte *)((uintptr_t)u + S_X));
            byte y = *((byte *)((uintptr_t)u + S_Y));
            set_region((int)x - 24, (int)y - 24, (int)x + 24, (int)y + 24); // set region around myself
            find_all_alive_units(ANY_UNITS);
            sort_in_region();
            byte o = *((byte *)((uintptr_t)u + S_OWNER));
            for (int ui = 0; ui < 16; ui++)
            {
                if (check_ally(o, ui))
                    sort_stat(S_OWNER, ui, CMP_NEQ);
            }
            if (units != 0)
            {
                byte mp = *((byte *)((uintptr_t)u + S_MANA));
                byte ord = *((byte *)((uintptr_t)u + S_ORDER));
                bool new_cast = (ord == ORDER_SPELL_ROT) || (ord == ORDER_SPELL_BLIZZARD) || (ord == ORDER_SPELL_INVIS) || (ord == ORDER_SPELL_ARMOR);
                WORD shl = *((WORD *)((uintptr_t)u + S_SHIELD));
                WORD inv = *((WORD *)((uintptr_t)u + S_INVIZ));
                if ((shl == 0) && (inv == 0))
                {
                    set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12); // set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui)) // enemy
                            sort_stat(S_OWNER, ui, CMP_NEQ);
                    }
                    if (units != 0)
                    {
                        struct GPOINT
                        {
                            WORD x;
                            WORD y;
                        } l;
                        l.x = *((WORD *)((uintptr_t)u + S_X));
                        l.y = *((WORD *)((uintptr_t)u + S_Y));
                        ((int (*)(int *, int, GPOINT *))F_ICE_SET_AI_ORDER)(u, AI_ORDER_DEFEND, &l);
                        set_stat(u, l.x, S_AI_DEST_X);
                        set_stat(u, l.y, S_AI_DEST_Y);
                    }
                    if (mp < 50)
                        new_cast = false;
                    else
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_INVIS)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(INVIS))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12); // set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_INVIZ, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_INVIS);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_POLYMORF)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(POLYMORPH))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18); // set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui)) // enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_POLYMORPH);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                        else if (id == U_DK)
                        {
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_UNHOLY)) != 0)
                            {
                                if (ord != ORDER_SPELL_ARMOR)
                                {
                                    if (mp >= get_manacost(UNHOLY_ARMOR))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12); // set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_ARMOR);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else
                                    new_cast = true;
                            }
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_HASTE)) != 0)
                            {
                                if (ord != ORDER_SPELL_HASTE)
                                {
                                    if (mp >= get_manacost(HASTE))
                                    {
                                        set_region((int)x - 16, (int)y - 16, (int)x + 16, (int)y + 16); // set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_NEQ);
                                        sort_stat(S_HASTE, 0, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_HASTE);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else
                                    new_cast = true;
                            }
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_COIL)) != 0)
                            {
                                if (ord != ORDER_SPELL_DRAINLIFE)
                                {
                                    if (mp >= get_manacost(COIL))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18); // set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        sort_stat(S_ANIMATION, ANIM_MOVE, CMP_NEQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui)) // enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            byte xx = *((byte *)((uintptr_t)unit[0] + S_X));
                                            byte yy = *((byte *)((uintptr_t)unit[0] + S_Y));
                                            give_order(u, xx, yy, ORDER_SPELL_DRAINLIFE);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    set_region((int)x - 50, (int)y - 50, (int)x + 50, (int)y + 50); // set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    sort_stat(S_ID, U_PEON, CMP_SMALLER_EQ);
                    sort_stat(S_ID, U_PEASANT, CMP_BIGGER_EQ);
                    sort_peon_loaded(0);
                    sort_hidden();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui))
                            sort_stat(S_OWNER, ui, CMP_NEQ); // enemy peons
                    }
                    if (units == 0)
                    {
                        find_all_alive_units(ANY_BUILDING);
                        sort_in_region();
                        sort_stat(S_ID, U_FORTRESS, CMP_SMALLER_EQ); // fortres castle stronghold keep
                        sort_stat(S_ID, U_KEEP, CMP_BIGGER_EQ);
                        for (int ui = 0; ui < 16; ui++)
                        {
                            if (check_ally(o, ui))
                                sort_stat(S_OWNER, ui, CMP_NEQ);
                        }
                        if (units == 0)
                        {
                            find_all_alive_units(ANY_BUILDING);
                            sort_in_region();
                            sort_stat(S_ID, U_GREAT_HALL, CMP_SMALLER_EQ);
                            sort_stat(S_ID, U_TOWN_HALL, CMP_BIGGER_EQ);
                            for (int ui = 0; ui < 16; ui++)
                            {
                                if (check_ally(o, ui))
                                    sort_stat(S_OWNER, ui, CMP_NEQ);
                            }
                        }
                    }
                    if (units != 0)
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_BLIZZARD)) != 0)
                            {
                                if (ord != ORDER_SPELL_BLIZZARD)
                                {
                                    if (mp >= get_manacost(BLIZZARD))
                                    {
                                        byte tx = *((byte *)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte *)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_BLIZZARD);
                                        new_cast = true;
                                    }
                                }
                                else
                                    new_cast = true;
                            }
                        }
                        if (id == U_DK)
                        {
                            if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_DD)) != 0)
                            {
                                if (ord != ORDER_SPELL_ROT)
                                {
                                    if (mp >= get_manacost(DEATH_AND_DECAY))
                                    {
                                        byte tx = *((byte *)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte *)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_ROT);
                                        new_cast = true;
                                    }
                                }
                                else
                                    new_cast = true;
                            }
                            else if ((*(DWORD *)(SPELLS_LEARNED + 4 * o) & (1 << L_WIND)) != 0)
                            {
                                if (!new_cast && (ord != ORDER_SPELL_WHIRLWIND))
                                {
                                    if (mp >= get_manacost(WHIRLWIND))
                                    {
                                        byte tx = *((byte *)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte *)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_WHIRLWIND);
                                        new_cast = true;
                                    }
                                }
                                else
                                    new_cast = true;
                            }
                        }
                    }
                }
                if (!new_cast)
                    return ((int (*)(int *))g_proc_0042757E)(u); // original
            }
        }
        else
            return ((int (*)(int *))g_proc_0042757E)(u); // original
        return 0;
    }
    else
        return ((int (*)(int *))g_proc_0042757E)(u); // original
}

PROC g_proc_00427FAE;
void ai_attack(int *u, int b, int a)
{
    if (ai_fixed)
    {
        byte mv = *((byte*)((uintptr_t)u + S_MOVEMENT_TYPE));
        if (mv == MOV_LAND)
        {
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            for (int i = 0; i < 16; i++)
            {
                int* p = (int*)(UNITS_LISTS + 4 * i);
                if (p)
                {
                    p = (int*)(*p);
                    while (p)
                    {
                        bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_MAGE) || (*((byte*)((uintptr_t)p + S_ID)) == U_DK));
                        if (f)
                        {
                            if (!check_unit_dead(p) && !check_unit_hidden(p))
                            {
                                byte ow = *((byte*)((uintptr_t)p + S_OWNER));
                                if (ow == o)
                                {
                                    if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                                    {
                                        WORD inv = *((WORD*)((uintptr_t)p + S_INVIZ));
                                        if (inv == 0)
                                        {
                                            byte aor = *((byte*)((uintptr_t)p + S_AI_ORDER));
                                            if (aor != AI_ORDER_ATTACK)
                                            {
                                                byte mp = *((byte*)((uintptr_t)p + S_MANA));
                                                if (mp >= 200)
                                                {
                                                    ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(p, AI_ORDER_ATTACK, a); // ai attack
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                    }
                }
            }

            find_all_alive_units(ANY_MEN);
            sort_stat(S_ID, U_GOBLINS, CMP_SMALLER_EQ);
            sort_stat(S_ID, U_DWARWES, CMP_BIGGER_EQ);
            sort_stat(S_OWNER, o, CMP_EQ);
            sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_NEQ); // not attack already
            for (int i = 0; i < units; i++)
            {
                ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, a); // ai attack
            }
        }
    }

    byte id = *((byte *)((uintptr_t)u + S_ID));
    if (id == U_GRIFON)
    {
        byte od = *((byte *)((uintptr_t)u + S_ORDER));
        if (od != ORDER_SPELL_RAISEDEAD)
            ((void (*)(int *, int, int))g_proc_00427FAE)(u, b, a); // original
    }
    else
        ((void (*)(int *, int, int))g_proc_00427FAE)(u, b, a); // original
}

PROC g_proc_00427FFF;
void ai_attack_nearest_enemy(int *u, WORD x, WORD y, int *t, int ordr)
{
    if (ai_fixed)
    {
        struct GPOINT
        {
            WORD x;
            WORD y;
        };
        GPOINT l;
        l.x = *((WORD *)((uintptr_t)t + S_X));
        l.y = *((WORD *)((uintptr_t)t + S_Y));
        ((int (*)(int *, int, GPOINT *))F_ICE_SET_AI_ORDER)(u, AI_ORDER_ATTACK, &l);
    }
    else
        ((void (*)(int *, WORD, WORD, int *, int))g_proc_00427FFF)(u, x, y, t, ordr); // original
}

void sap_behaviour()
{
    for (int i = 0; i < 16; i++)
    {
        int *p = (int *)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int *)(*p);
            while (p)
            {
                bool f = ((*((byte *)((uintptr_t)p + S_ID)) == U_DWARWES) || (*((byte *)((uintptr_t)p + S_ID)) == U_GOBLINS));
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte *)((uintptr_t)p + S_OWNER));
                        if ((*(byte *)(CONTROLER_TYPE + o) == C_COMP))
                        {
                            byte ord = *((byte *)((uintptr_t)p + S_ORDER));
                            byte x = *((byte *)((uintptr_t)p + S_X));
                            byte y = *((byte *)((uintptr_t)p + S_Y));
                            if ((ord != ORDER_DEMOLISH) && (ord != ORDER_DEMOLISH_NEAR) && (ord != ORDER_DEMOLISH_AT))
                            {
                                set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12); // set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui)) // only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int *)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5); // set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui)) // only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int *)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 1, (int)y - 1, (int)x + 1, (int)y + 1); // set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui)) // only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int *)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int *, int, int, int *, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                            }
                        }
                    }
                }
                p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void unstuk()
{
    for (int i = 0; i < 16; i++)
    {
        int *p = (int *)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int *)(*p);
            while (p)
            {
                byte id = *((byte *)((uintptr_t)p + S_ID));
                byte mov = *((byte *)((uintptr_t)p + S_MOVEMENT_TYPE));
                byte ord = *((byte *)((uintptr_t)p + S_ORDER));
                byte aord = *((byte *)((uintptr_t)p + S_AI_ORDER));
                bool f = ((((id < U_CRITTER) && (mov == MOV_LAND)) && !check_unit_preplaced(p) && ((ord == ORDER_STOP) || (ord == ORDER_MOVE_PATROL)) && (aord == AI_ORDER_ATTACK)) ||
                          ((id == U_PEASANT) || (id == U_PEON)));
                // if (*(byte*)GB_MULTIPLAYER)if (!((id == U_PEASANT) || (id == U_PEON)))f = false;
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        if (!check_unit_hidden(p))
                        {
                            byte o = *((byte *)((uintptr_t)p + S_OWNER));
                            if ((*(byte *)(CONTROLER_TYPE + o) == C_COMP))
                            {
                                byte st = *((byte *)((uintptr_t)p + S_NEXT_FIRE));
                                byte frm = *((byte *)((uintptr_t)p + S_FRAME));
                                byte pfrm = *((byte *)((uintptr_t)p + S_NEXT_FIRE + 1));
                                if (st == 0)
                                {
                                    byte map = *(byte *)MAP_SIZE - 1;
                                    byte x = *((byte *)((uintptr_t)p + S_X));
                                    byte y = *((byte *)((uintptr_t)p + S_Y));
                                    int xx = 0, yy = 0, dir = 0;
                                    xx += x;
                                    yy += y;
                                    dir = ((int (*)())F_NET_RANDOM)();
                                    dir %= 8;
                                    if (dir == 0)
                                    {
                                        if (yy > 0)
                                            yy -= 1;
                                    }
                                    if (dir == 1)
                                    {
                                        if (yy > 0)
                                            yy -= 1;
                                        if (xx < map)
                                            xx += 1;
                                    }
                                    if (dir == 2)
                                    {
                                        if (xx < map)
                                            xx += 1;
                                    }
                                    if (dir == 3)
                                    {
                                        if (xx < map)
                                            xx += 1;
                                        if (yy < map)
                                            yy += 1;
                                    }
                                    if (dir == 4)
                                    {
                                        if (yy < map)
                                            yy += 1;
                                    }
                                    if (dir == 5)
                                    {
                                        if (yy < map)
                                            yy += 1;
                                        if (xx > 0)
                                            xx -= 1;
                                    }
                                    if (dir == 6)
                                    {
                                        if (xx > 0)
                                            xx -= 1;
                                    }
                                    if (dir == 7)
                                    {
                                        if (xx > 0)
                                            xx -= 1;
                                        if (yy > 0)
                                            yy -= 1;
                                    }
                                    give_order(p, xx % 256, yy % 256, ORDER_MOVE);
                                    st = 255;
                                }
                                if (st > 0)
                                    st -= 1;
                                if (frm != pfrm)
                                    st = 255;
                                set_stat(p, st, S_NEXT_FIRE);
                                set_stat(p, frm, S_NEXT_FIRE + 1);
                            }
                        }
                    }
                }
                p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void paladin()
{
    for (int ii = 0; ii < 16; ii++)
    {
        int *p = (int *)(UNITS_LISTS + 4 * ii);
        if (p)
        {
            p = (int *)(*p);
            while (p)
            {
                bool f = ((*((byte *)((uintptr_t)p + S_ID)) == U_PALADIN) ||
                          (*((byte *)((uintptr_t)p + S_ID)) == U_UTER) ||
                          (*((byte *)((uintptr_t)p + S_ID)) == U_TYRALYON));
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte *)((uintptr_t)p + S_OWNER));
                        if (((*(byte *)(SPELLS_LEARNED + 4 * o) & (1 << L_HEAL)) != 0) &&
                            ((*(byte *)(SPELLS_LEARNED + 4 * o) & (1 << L_GREATER_HEAL)) != 0))
                        // if player learned heal and autoheal
                        {
                            byte x = *((byte *)((uintptr_t)p + S_X));
                            byte y = *((byte *)((uintptr_t)p + S_Y));
                            set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5); // set region around myself
                            find_all_alive_units(ANY_MEN);
                            sort_in_region();
                            sort_hidden();
                            sort_fleshy();   // fleshy units (not heal cata and ships)
                            sort_full_hp();  // if unit hp not full
                            sort_self(p);    // not heal self
                            sort_order_hp(); // heal lovest hp first
                            for (int ui = 0; ui < 16; ui++)
                            {
                                if (!check_ally(o, ui)) // only allied units
                                    sort_stat(S_OWNER, ui, 1);
                            }
                            byte cost = *(byte *)(MANACOST + 2 * GREATER_HEAL); // 2* cause manacost is WORD
                            for (int i = 0; i < units; i++)
                            {
                                byte mp = *((byte *)((uintptr_t)p + S_MANA)); // paladin mp
                                if (mp >= cost)
                                {
                                    byte id = *((byte *)((uintptr_t)unit[i] + S_ID)); // unit id
                                    WORD mhp = *(WORD *)(UNIT_HP_TABLE + 2 * id);     // max hp
                                    WORD hp = *((WORD *)((uintptr_t)unit[i] + S_HP)); // unit hp
                                    WORD shp = mhp - hp;                              // shortage of hp
                                    while (!(mp >= (shp * cost)) && (shp > 0))
                                        shp -= 1;
                                    if (shp > 0) // if can heal at least 1 hp
                                    {
                                        heal(unit[i], (byte)shp, 0);
                                        mp -= shp * cost;
                                        *((byte *)((uintptr_t)p + S_MANA)) = mp;
                                        WORD xx = *((WORD *)((uintptr_t)unit[i] + S_DRAW_X));
                                        WORD yy = *((WORD *)((uintptr_t)unit[i] + S_DRAW_Y));
                                        bullet_create(xx + 16, yy + 16, B_HEAL); // create heal effect
                                        if (shp >= 3)
                                            ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_HEAL); // heal sound
                                    }
                                }
                                else
                                    i = units;
                            }
                        }
                    }
                }
                p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

bool devotion_aura(int *trg, byte id)
{
    byte tid = *((byte *)((uintptr_t)trg + S_ID)); // unit id
    if (tid < U_FARM)                              // not buildings
    {
        byte x = *((byte *)((uintptr_t)trg + S_X));
        byte y = *((byte *)((uintptr_t)trg + S_Y));
        set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5); // set region around myself
        find_all_alive_units(id);
        sort_in_region();
        byte o = *((byte *)((uintptr_t)trg + S_OWNER));
        for (int ui = 0; ui < 16; ui++)
        {
            if (!check_ally(o, ui))
                sort_stat(S_OWNER, ui, CMP_NEQ);
        }
        return units != 0;
    }
    else
        return false;
}

void make_rain()
{
    if (can_rain)
    {
        for (int i = 0; i < raindrops_amount; i++)
        {
            if (raindrops[i].l != 0)
            {
                int t = 0;
                int ms = raindrops_speed;
                while (ms > 0)
                {
                    t = raindrops[i].x2;
                    raindrops[i].x2 += abs(raindrops[i].x2 - raindrops[i].x1);
                    raindrops[i].x1 = t;
                    ms--;
                }
                ms = raindrops_speed;
                while (ms > 0)
                {
                    t = raindrops[i].y2;
                    raindrops[i].y2 += abs(raindrops[i].y2 - raindrops[i].y1);
                    raindrops[i].y1 = t;
                    ms--;
                }
                if (raindrops[i].l > 0)
                    raindrops[i].l--;
                if ((raindrops[i].x1 >= m_maxx) || (raindrops[i].y1 >= m_maxy))
                    raindrops[i].l = 0;
            }
        }

        int n = 1 + rand() % raindrops_density;
        int last = 0;
        for (int k = 0; k < n; k++)
        {
            int c = 0;
            for (int i = last; i < raindrops_amount; i++)
            {
                last = i;
                if (raindrops[i].l == 0)
                {
                    c = i;
                    break;
                }
            }
            if (last == (raindrops_amount - 1))
            {
                c = rand() % raindrops_amount;
            }
            if (rand() % 2 == 0)
            {
                raindrops[c].x1 = m_minx + (rand() % my_max(1, (m_maxx - m_minx - raindrops_size)));
                raindrops[c].y1 = m_miny;
                raindrops[c].x2 = raindrops[c].x1 + raindrops_size_x + (rand() % my_max(1, raindrops_align_x));
                raindrops[c].y2 = raindrops[c].y1 + raindrops_size_y + (rand() % my_max(1, raindrops_align_y));
            }
            else
            {
                raindrops[c].x1 = m_minx;
                raindrops[c].y1 = m_miny + (rand() % my_max(1, (m_maxy - m_miny - raindrops_size)));
                raindrops[c].x2 = raindrops[c].x1 + raindrops_size_x + (rand() % my_max(1, raindrops_align_x));
                raindrops[c].y2 = raindrops[c].y1 + raindrops_size_y + (rand() % my_max(1, raindrops_align_y));
            }
            raindrops[c].l = 20 + (rand() % my_max(1, (m_maxy / my_max(1, (raindrops_size / 2)))));
        }

        if (raindrops_thunder != 0)
        {
            if (raindrops_thunder_timer > 0)
                raindrops_thunder_timer--;
            if (raindrops_thunder_timer == 0)
            {
                if (raindrops_thunder_gradient == THUNDER_GRADIENT)
                {
                    save_palette();
                }
                if (raindrops_thunder_gradient > 0)
                    raindrops_thunder_gradient--;
                change_palette(raindrops_thunder_gradient > (THUNDER_GRADIENT / 2));
                if (raindrops_thunder_gradient == (THUNDER_GRADIENT / 2))
                {
                    ((void (*)(WORD))F_WAR2_SOUND_PLAY)(81);
                }
                if (raindrops_thunder_gradient == 0)
                {
                    raindrops_thunder_timer = (raindrops_thunder / 2) + (rand() % (1 + (raindrops_thunder / 2)));
                    raindrops_thunder_gradient = THUNDER_GRADIENT;
                    reset_palette();
                }
            }
        }
    }
}

void bldg_build_queue()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                byte id = *((byte*)((uintptr_t)p + S_ID));
                bool f = (id >= U_FARM);
                if (f)
                {
                    if (!check_unit_dead(p) && check_unit_complete(p))
                    {
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        if ((*(byte*)(CONTROLER_TYPE + o) == C_PLAYER))
                        {
                            if ((*((byte*)((uintptr_t)p + S_FLAGS1)) & UF_BUILD_ON) == 0)
                            {
                                WORD uid = unit_fixup(p);
                                if (build_queue[uid * 16] != 255)
                                {
                                    int s = 0;
                                    if (build_queue[uid * 16 + 1] == 1)//research spell
                                    {
                                        if (bldg_check_spell_learned(build_queue[uid * 16], o))s = 1;
                                    }
                                    if (build_queue[uid * 16 + 1] == 2)//research tech
                                    {
                                        if (bldg_check_tech_learned(build_queue[uid * 16], o))s = 1;
                                    }
                                    if (build_queue[uid * 16 + 1] == 3)//upgrade building
                                    {
                                        if (build_queue[uid * 16] == id)s = 1;
                                        if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER))s = 1;
                                    }
                                    if (s == 0)s = building_start_build(p, build_queue[uid * 16], build_queue[uid * 16 + 1]);
                                    if (s == 1)//start new build
                                    {
                                        int k = 0;
                                        while (k < 7)//move copy queue 
                                        {
                                            build_queue[uid * 16 + k * 2] = build_queue[uid * 16 + k * 2 + 2];
                                            build_queue[uid * 16 + k * 2 + 1] = build_queue[uid * 16 + k * 2 + 3];
                                            k++;
                                        }
                                        build_queue[uid * 16 + 7 * 2] = 255;//remove last
                                        build_queue[uid * 16 + 7 * 2 + 1] = 255;
                                    }
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void unit_order_queue()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = (*((byte*)((uintptr_t)p + S_ID)) < U_FARM);
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        if ((*(byte*)(CONTROLER_TYPE + o) == C_PLAYER))
                        {
                            byte od = *((byte*)((uintptr_t)p + S_ORDER));
                            byte nod = *((byte*)((uintptr_t)p + S_NEXT_ORDER));
                            if ((od == ORDER_STOP) && (nod == ORDER_NONE))
                            {
                                WORD uid = unit_fixup(p);
                                if (order_queue[uid * 48] != 255)
                                {
                                    if (order_type_target(order_queue[uid * 48]))
                                        give_order_target(p, 0, 0, unit_unfixup((WORD)(order_queue[uid * 48 + 1] + 256 * order_queue[uid * 48 + 2])), order_queue[uid * 48]);
                                    else
                                        give_order_target(p, order_queue[uid * 48 + 1], order_queue[uid * 48 + 2], NULL, order_queue[uid * 48]);

                                    int k = 0;
                                    while (k < 15)//move copy queue 
                                    {
                                        order_queue[uid * 48 + k * 3] = order_queue[uid * 48 + k * 3 + 3];
                                        order_queue[uid * 48 + k * 3 + 1] = order_queue[uid * 48 + k * 3 + 4];
                                        order_queue[uid * 48 + k * 3 + 2] = order_queue[uid * 48 + k * 3 + 5];
                                        k++;
                                    }
                                    order_queue[uid * 48 + 15 * 3] = 255;//remove last
                                    order_queue[uid * 48 + 15 * 3 + 1] = 255;
                                    order_queue[uid * 48 + 15 * 3 + 2] = 255;
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void food_limit(int l)
{
    byte p = *(byte*)LOCAL_PLAYER;
    int farm = 4;
    if ((*(DWORD*)(SPELLS_LEARNED + 4 * p) & (1 << L_HASTE)) != 0)farm += 1;
    int food = farm * get_val(FARM, p) + get_val(TH1, p) + get_val(TH2, p) + get_val(TH3, p);
    if (food > 200)food = 200;
    if (food > l)food = l;
    char buf[] = "\x0\x0";
    buf[0] = (byte)(food % 256);
    buf[1] = (byte)(food / 256);
    PATCH_SET((char*)(FOOD_LIMIT + 2 * p), buf);
}

int move_line_break = 0;

PROC g_proc_0045271B;
void update_spells()
{
    game_started = true;
    ((void (*)())g_proc_0045271B)(); // original
    // this function called every game tick
    // you can place your self-writed functions here in the end if need

    move_line_break--;
    if (move_line_break <= 0)move_line_break = 24;

    food_limit(200);

    make_rain();

    if (A_autoheal)
        paladin();

    if (saveload_fixed)
        tech_reinit();
    if (ai_fixed)
    {
        unstuk();
        sap_behaviour();
    }
    if (vizs_n > 0)
    {
        for (int i = 0; i < vizs_n; i++)
        {
            viz_area(vizs_areas[i].x, vizs_areas[i].y, vizs_areas[i].p, vizs_areas[i].s);
        }
    }

    find_all_alive_units(ANY_BUILDING);
    for (int i = 0; i < units; i++)
    {
        byte sf = *((byte*)((uintptr_t)unit[i] + S_SEQ_FLAG));
        sf |= 0x20;
        set_stat(unit[i], sf, S_SEQ_FLAG);
    }

    bldg_build_queue();
    unit_order_queue();

    ((void (*)())F_STATUS_REDRAW)();//status redraw

    inval_game_view();
}

void seq_change(int *u, byte tt)
{
    byte t = tt;
    if (t == 1)
    {
        byte t = *((byte *)((uintptr_t)u + S_ANIMATION_TIMER));
        byte id = *((byte *)((uintptr_t)u + S_ID));
        byte a = *((byte *)((uintptr_t)u + S_ANIMATION));
        byte f = *((byte *)((uintptr_t)u + S_FRAME));
        byte o = *((byte *)((uintptr_t)u + S_OWNER));
        if ((id == U_ARCHER) || (id == U_RANGER))
        {
            if (a == ANIM_ATTACK)
            {
                if (t > 1)
                    t -= 1;
                set_stat(u, 1, S_ATTACK_COUNTER);
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        if ((id == U_KNIGHT) && (o == *(byte*)LOCAL_PLAYER))
        {
            if (a == ANIM_MOVE)
            {
                if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_FLAME_SHIELD)) == 0)
                {
                    t = 2;
                }
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
    }
}

PROC g_proc_004522B9;
int seq_run(int *u)
{
    byte t = *((byte *)((uintptr_t)u + S_ANIMATION_TIMER));
    bool f = true;
    int original = 0;
    byte id = *((byte *)((uintptr_t)u + S_ID));
    if ((id == U_PEON) || (id == U_PEASANT))
    {
        byte pf = *((byte *)((uintptr_t)u + S_PEON_FLAGS));
        byte sn = *((byte *)((uintptr_t)u + S_SEQ_SOUND));
        if (pf & 1)
        {
            if ((sn >= 34) && (sn <= 37))
            {
                int oid = sn - 1;
                def_name_seq = (void *)*(int *)(SOUNDS_FILES_LIST + 8 + 24 * oid);
                def_sound_seq = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid); // save default
                patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)kir_names[sn - 34]);
                patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)kir_sounds[sn - 34]);
                original = ((int (*)(int *))g_proc_004522B9)(u); // original
                kir_sounds[sn - 34] = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid);
                patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)def_sound_seq);
                patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)def_name_seq); // restore default
                f = false;
            }
        }
    }
    else if (id == U_BALLISTA)
    {
        int r = ((int (*)())F_NET_RANDOM)() % 2;
        int oid = *((byte *)((uintptr_t)u + S_SEQ_SOUND)) - 1;
        def_name_seq = (void *)*(int *)(SOUNDS_FILES_LIST + 8 + 24 * oid);
        def_sound_seq = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid); // save default
        patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)c_names[r]);
        patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)c_sounds[r]);
        original = ((int (*)(int *))g_proc_004522B9)(u); // original
        c_sounds[r] = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * oid);
        patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)def_sound_seq);
        patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)def_name_seq); // restore default
        f = false;
    }
    if (f)
        original = ((int (*)(int *))g_proc_004522B9)(u); // original
    seq_change(u, t);
    return original;
}

bool red_at;
bool green_at;
bool violet_at;

PROC g_proc_00409F3B,                 // action
    g_proc_0040AF70, g_proc_0040AF99, // demolish
    g_proc_0041038E, g_proc_00410762, // bullet
    g_proc_004428AD;                  // spell
char fdmg = 0;                        // final damage
void damage(int *atk, int *trg, char dmg)
{
    fdmg = dmg;
    if ((trg != NULL) && (atk != NULL))
    {
        if (!check_unit_dead(trg))
        {
            byte aid = *((byte *)((uintptr_t)atk + S_ID)); // attacker id
            byte tid = *((byte *)((uintptr_t)trg + S_ID)); // target id
            byte dmg2 = dmg;                               // can deal damage by default
            int i = 0;
            while (i < 255)
            {
                if ((tid == ut[i]) && (aid != ua[i]))
                {
                    dmg2 = 0; // canot deal damage
                }
                if ((tid == ut[i]) && (aid == ua[i])) // check if only some certain units can attack that unit
                {
                    dmg2 = dmg; // can deal damage
                    i = 255;
                }
                i++;
                if (ua[i] == 255) // pairs must go in a row
                {
                    i = 255;
                }
            }
            if (*((WORD *)((uintptr_t)trg + S_SHIELD)) != 0)
                dmg2 = 0;
            fdmg = dmg2;
            if (fdmg != 0)
            {
                if (tid == U_FOOTMAN)
                {
                    byte to = *((byte *)((uintptr_t)trg + S_OWNER));
                    to = *(byte *)(CONTROLER_TYPE + to);
                    if (to == C_COMP)
                    {
                        byte pf = *((byte *)((uintptr_t)trg + S_PEON_FLAGS));
                        pf |= PEON_HARVEST_GOLD;
                        set_stat(trg, pf, S_PEON_FLAGS);
                    }
                    byte pf = *((byte *)((uintptr_t)trg + S_PEON_FLAGS));
                    if (pf & PEON_HARVEST_GOLD)
                    {
                        dmg2 = dmg2 * 0.75; // 25% block
                        fdmg = (dmg2 == 0) ? 1 : dmg2;
                    }
                }
                bool f = false;
                for (int i = 0; i < 255; i++)
                {
                    if ((m_devotion[i] != 255) && (!f))
                        f = devotion_aura(trg, m_devotion[i]);
                    else
                        i = 256;
                }
                if (f) // defence
                {
                    dmg2 = fdmg;
                    if (dmg2 > 3)
                        dmg2 -= 3;
                    else
                        dmg2 = 0;
                    fdmg = dmg2;
                }
            }
			
			if (fdmg > 0)
			{
				byte ao = *((byte *)((uintptr_t)atk + S_OWNER));
				byte to = *((byte *)((uintptr_t)trg + S_OWNER));
				if (ao == *(byte*)LOCAL_PLAYER)
				{
					if (to == P_RED)
					{
						red_at = true;
					}
				}
			}
			
			if (fdmg > 0)
			{
				byte ao = *((byte *)((uintptr_t)atk + S_OWNER));
				byte to = *((byte *)((uintptr_t)trg + S_OWNER));
				if (ao == *(byte*)LOCAL_PLAYER)
				{
					if (to == P_GREEN)
					{
						green_at = true;
					}
				}
			}
			
			if (fdmg > 0)
			{
				byte ao = *((byte *)((uintptr_t)atk + S_OWNER));
				byte to = *((byte *)((uintptr_t)trg + S_OWNER));
				if (ao == *(byte*)LOCAL_PLAYER)
				{
					if (to == P_VIOLET)
					{
						violet_at = true;
					}
				}
			}
			
            WORD hp = *((WORD *)((uintptr_t)trg + S_HP)); // unit hp
            if ((tid < U_FARM) && (fdmg >= hp))
            {
                if ((aid != U_HTRANSPORT) || (aid != U_OTRANSPORT))
                {
                    bool killed = true;
                    if (killed)
                    {
                        byte k = *((byte *)((uintptr_t)atk + S_KILLS));
                        if (k < 255)
                            k += 1;
                        set_stat(atk, (int)k, S_KILLS);
                    }
                }
            }
        }
    }
}

void damage1(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_00409F3B)(atk, trg, fdmg);
}

void damage2(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_0041038E)(atk, trg, fdmg);
}

void damage3(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_0040AF70)(atk, trg, fdmg);
}

void damage4(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_0040AF99)(atk, trg, fdmg);
}

void damage5(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_00410762)(atk, trg, fdmg);
}

void damage6(int *atk, int *trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int *, int *, char))g_proc_004428AD)(atk, trg, fdmg);
}

byte at_def_str = 0;
byte at_def_prc = 0;
byte tr_def_arm = 0;

void change_str(byte id, byte str)
{
    char buf[] = "\x0";
    buf[0] = str;
    PATCH_SET((char*)(UNIT_STRENGTH_TABLE + id), buf);
}

void change_prc(byte id, byte prc)
{
    char buf[] = "\x0";
    buf[0] = prc;
    PATCH_SET((char*)(UNIT_PIERCE_TABLE + id), buf);
}

void change_arm(byte id, byte arm)
{
    char buf[] = "\x0";
    buf[0] = arm;
    PATCH_SET((char*)(UNIT_ARMOR_TABLE + id), buf);
}

void attacker_change_damage(int* u)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    at_def_str = *(byte*)(UNIT_STRENGTH_TABLE + id);
    at_def_prc = *(byte*)(UNIT_PIERCE_TABLE + id);

    byte ow = *((byte*)((uintptr_t)u + S_OWNER));
    if (ow == *(byte*)LOCAL_PLAYER)
    {
        if (id == U_KNIGHT)
        {
            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_BLIZZARD)) != 0)
            {
                change_prc(id, at_def_prc + 3);
            }
        }
    }

    /*
    if (id == U_FOOTMAN)
    {
        change_prc(id, 0);
        byte c = *((byte*)((uintptr_t)u + S_COLOR));
        if (c == P_RED)
        {
            change_str(id, 10);
        }
        if (c == P_BLUE)
        {
            change_str(id, 20);
        }
    }

    if (id == U_ARCHER)
    {
        byte c = *((byte*)((uintptr_t)u + S_COLOR));
        if (c == P_RED)
        {
            change_str(id, 10);
            change_prc(id, 0);
        }
        if (c == P_BLUE)
        {
            change_str(id, 0);
            change_prc(id, 10);
        }
    }

    if (id == U_BALLISTA)
    {
        byte c = *((byte*)((uintptr_t)u + S_COLOR));
        if (c == P_RED)
        {
            change_str(id, 30);
            change_prc(id, 0);
        }
        if (c == P_BLUE)
        {
            change_str(id, 0);
            change_prc(id, 50);
        }
    }
    */
}

void target_change_armor(int* u)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    tr_def_arm = *(byte*)(UNIT_ARMOR_TABLE + id);

    byte ow = *((byte*)((uintptr_t)u + S_OWNER));
    if (ow == *(byte*)LOCAL_PLAYER)
    {
        if (id == U_KNIGHT)
        {
            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_RUNES)) != 0)
            {
                change_arm(id, tr_def_arm + 4);
            }
        }
    }

    /*
    if (id == U_KNIGHT)
    {
        byte c = *((byte*)((uintptr_t)u + S_COLOR));
        if (c == P_RED)
        {
            change_arm(id, 10);
        }
        if (c == P_BLUE)
        {
            change_arm(id, 20);
        }
    }
    */
}

void attacker_default_damage(int* u)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    change_str(id, at_def_str);
    change_prc(id, at_def_prc);
}

void target_default_armor(int* u)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    change_arm(id, tr_def_arm);
}

PROC g_proc_00409F1E;
byte damage_calc_damage_action(int* at)
{
    int* tr = (int*)*((int*)((uintptr_t)at + S_ORDER_UNIT_POINTER));
    attacker_change_damage(at);
    if (tr)target_change_armor(tr);
    byte original = ((byte(*)(int*))g_proc_00409F1E)(at);
    attacker_default_damage(at);
    if (tr)target_default_armor(tr);
    return original;
}

PROC g_proc_0040FBE7;
byte damage_calc_damage_bullet(int* at)
{
    int* tr = (int*)*((int*)((uintptr_t)at + S_ORDER_UNIT_POINTER));
    attacker_change_damage(at);
    if (tr)target_change_armor(tr);
    byte original = ((byte(*)(int*))g_proc_0040FBE7)(at);
    attacker_default_damage(at);
    if (tr)target_default_armor(tr);
    return original;
}

PROC g_proc_0040FBE0;
WORD damage_calc_sum(int* at)
{
    attacker_change_damage(at);
    WORD original = ((WORD(*)(int*))g_proc_0040FBE0)(at);
    attacker_default_damage(at);
    return original;
}

PROC g_proc_00410733;
byte damage_area_get_target_armor(int* tr)
{
    target_change_armor(tr);
    byte original = ((byte(*)(int*))g_proc_00410733)(tr);
    target_default_armor(tr);
    return original;
}

PROC g_proc_0040AFD4;
void damage_bullet_tower_create(int* at, int* tr)
{
    attacker_change_damage(at);
    target_change_armor(tr);
    ((void(*)(int*, int*))g_proc_0040AFD4)(at, tr);
    attacker_default_damage(at);
    target_default_armor(tr);
}

void tower_set_target(int *p, int x, int y)
{
    set_stat(p, 0, S_RETARGET_X1 - 2);
    set_stat(p, 0, S_RETARGET_X1 - 1);
    int *u = NULL;
    set_region(x - 3, y - 3, x, y);
    find_all_alive_units(ANY_BUILDING_4x4);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)
        u = unit[0];
    set_region(x - 2, y - 2, x, y);
    find_all_alive_units(ANY_BUILDING_3x3);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)
        u = unit[0];
    set_region(x - 1, y - 1, x, y);
    find_all_alive_units(ANY_BUILDING_2x2);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)
        u = unit[0];
    set_region(x, y, x, y);
    find_all_alive_units(ANY_UNITS);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)
        u = unit[0];
    if (u)
    {
        int fx = ((int (*)(int *, int))F_UNIT_FIXUP)(u, 1); // fixup save
        set_stat(p, (int)fx % 256, S_RETARGET_X1 - 2);
        set_stat(p, (int)fx / 256, S_RETARGET_X1 - 1);
    }
}

PROC g_proc_0043BAE1;
void rc_snd(int *p)
{
    if (*((byte *)((uintptr_t)p + S_ID)) < U_FARM)
        ((void (*)(int *))g_proc_0043BAE1)(p);
}

PROC g_proc_0043B943;
void rc_build_click(int *p, int x, int y, int *t, int a)
{
    byte id = *(byte *)((uintptr_t)p + S_ID);
    if (id >= U_FARM)
    {
        if (A_rally)
        {
            bool f = true;
            byte xx = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 2));
            byte yy = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 1));
            if ((xx & 128) != 0)
            {
                xx &= ~128;
                if ((xx == x) && (yy == y))
                {
                    set_stat(p, 0, S_RETARGET_X1 - 2);
                    set_stat(p, 0, S_RETARGET_X1 - 1);
                    set_stat(p, 0, S_RETARGET_ORDER - 1);
                    f = false;
                    sound_play_from_file(0, (DWORD)rally_name, rally_sound);
                }
            }
            if (f)
            {
                set_stat(p, x | 128, S_RETARGET_X1 - 2);
                set_stat(p, y, S_RETARGET_X1 - 1);
                set_stat(p, 0, S_RETARGET_ORDER - 1);
                bool h = false;
                set_region(x - 3, y - 3, x, y);
                find_all_alive_units(U_MINE);
                sort_in_region();
                if (units != 0)h = true;
                find_all_alive_units(ANY_BUILDING);
                sort_stat(S_ID, U_HPLATFORM, CMP_BIGGER_EQ);
                sort_stat(S_ID, U_OPLATFORM, CMP_SMALLER_EQ);
                sort_in_region();
                if (units != 0)h = true;

                struct GPOINT
                {
                    WORD x;
                    WORD y;
                };
                GPOINT l;
                l.x = (WORD)x;
                l.y = (WORD)y;
                int tr1 = ((int (*)(GPOINT*))F_TILE_IS_TREE)(&l);
                int tr2 = ((int (*)(GPOINT*))F_TILE_IS_CHOPPING_TREE)(&l);

                char buf[] = "\xFD";
                PATCH_SET((char*)ROCKS_TILE_IS_TREE, buf);
                int tr3 = ((int (*)(GPOINT*))F_TILE_IS_TREE)(&l);
                buf[0] = '\xFE';
                PATCH_SET((char*)ROCKS_TILE_IS_TREE, buf);
                char buf2[] = "\xFF";
                PATCH_SET((char*)ROCKS_TILE_IS_CHOPPING_TREE, buf2);
                int tr4 = ((int (*)(GPOINT*))F_TILE_IS_CHOPPING_TREE)(&l);
                buf2[0] = '\xFC';
                PATCH_SET((char*)ROCKS_TILE_IS_CHOPPING_TREE, buf2);

                if ((tr1 == 1) || (tr2 == 1) || (tr3 == 1) || (tr4 == 1))h = true;
                if (h)
                {
                    if ((tr3 == 1) || (tr4 == 1))set_stat(p, 2, S_RETARGET_ORDER - 1);
                    else set_stat(p, 1, S_RETARGET_ORDER - 1);
                }
                sound_play_from_file(0, (DWORD)rally2_name, rally2_sound);
            }
            return;
        }
    }
    else
        ((void (*)(int *, int, int, int *, int))g_proc_0043B943)(p, x, y, t, a); // original
}

void rc_jmp(bool b)
{
    if (b)
    {
        char r[] = "\xf\x84\xa2\x0\x0\x0";
        PATCH_SET((char *)RIGHT_CLICK_1, r);
        patch_ljmp((char *)RIGHT_CLICK_CODE_CAVE, (char *)RIGHT_CLICK_2);
    }
    else
    {
        char r[] = "\xf\x84\x8b\x0\x0\x0";
        PATCH_SET((char *)RIGHT_CLICK_1, r);
    }
}

void brclik(bool b)
{
    if (b)
    {
        char r[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char *)RIGHT_CLICK_ALLOW_BUILDINGS, r);
        rc_jmp(true);
    }
    else
    {
        char r[] = "\xf\x84\x26\x01\x0\x0";
        PATCH_SET((char *)RIGHT_CLICK_ALLOW_BUILDINGS, r);
        rc_jmp(false);
    }
}

PROC g_proc_0040AFBF;
int *tower_find_attacker(int *p)
{
    int *tr = NULL;
    byte id = *((byte *)((uintptr_t)p + S_ID));
    if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER))
    {
        byte a1 = *((byte *)((uintptr_t)p + S_RETARGET_X1 - 2));
        byte a2 = *((byte *)((uintptr_t)p + S_RETARGET_X1 - 1));
        tr = (int *)(a1 + 256 * a2);
        tr = ((int *(*)(int *, int))F_UNIT_FIXUP)(tr, 0); // fixup load
        if (tr)
        {
            if (!check_unit_near_death(tr) && !check_unit_dead(tr) && !check_unit_hidden(tr))
            {
                int a = ((int (*)(int *, int *))F_ATTACK_CAN_HIT)(p, tr);
                if (a != 0)
                {
                    byte id = *((byte *)((uintptr_t)tr + S_ID));
                    byte szx = *(byte *)(UNIT_SIZE_TABLE + 4 * id);
                    byte szy = *(byte *)(UNIT_SIZE_TABLE + 4 * id + 2);
                    byte idd = *((byte *)((uintptr_t)p + S_ID));
                    byte rng = *(byte *)(UNIT_RANGE_TABLE + idd);
                    byte ms = *(byte *)MAP_SIZE;
                    byte xx = *((byte *)((uintptr_t)tr + S_X));
                    byte yy = *((byte *)((uintptr_t)tr + S_Y));
                    byte x1 = *((byte *)((uintptr_t)p + S_X));
                    byte y1 = *((byte *)((uintptr_t)p + S_Y));
                    byte x2 = x1;
                    byte y2 = y1;
                    if (x1 < rng)
                        x1 = 0;
                    else
                        x1 -= rng;
                    if (y1 < rng)
                        y1 = 0;
                    else
                        y1 -= rng;
                    if ((x2 + rng + 1) > ms)
                        x2 = ms;
                    else
                        x2 += rng + 1;
                    if ((y2 + rng + 1) > ms)
                        y2 = ms;
                    else
                        y2 += rng + 1;
                    if (!((xx >= x1) && (xx <= x2) && (yy >= y1) && (yy <= y2)))
                        tr = NULL;
                }
            }
            else
                tr = NULL;
        }
    }
    if (!tr)
        return ((int *(*)(int *))g_proc_0040AFBF)(p); // original
    else
    {
        return tr;
    }
}

PROC g_proc_0040DF71;
int *bld_unit_create(int a1, int a2, int a3, byte a4, int *a5)
{
    int *b = (int *)*(int *)UNIT_RUN_UNIT_POINTER;
    int *u = ((int *(*)(int, int, int, byte, int *))g_proc_0040DF71)(a1, a2, a3, a4, a5);
    if (b)
    {
        if (u)
        {
            byte idd = *((byte *)((uintptr_t)u + S_ID));
            if (idd == U_DANATH)
            {
                find_all_alive_units(U_DANATH);
                if (units > 1)
                {
                    unit_remove(u);
                    return NULL;
                }
                else
                    set_stat(u, P_ORANGE, S_COLOR);
            }
            byte x = *((byte *)((uintptr_t)b + S_RETARGET_X1 - 2));
            byte y = *((byte *)((uintptr_t)b + S_RETARGET_X1 - 1));
            byte ro = *((byte *)((uintptr_t)b + S_RETARGET_ORDER - 1));
            byte bp = x & 128;
            if (bp != 0)
            {
                x &= ~128;
                byte uid = *((byte *)((uintptr_t)u + S_ID));
                byte o = ORDER_ATTACK_AREA;
                if ((uid == U_PEON) || (uid == U_PEASANT) || (uid == U_HTANKER) || (uid == U_OTANKER))
                {
                    if ((ro == 1) || (ro == 2))
                    {
                        o = ORDER_HARVEST;
                        if (ro == 2)
                        {
                            byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
                            pf |= 1;
                            set_stat(u, pf, S_PEON_FLAGS);
                        }
                    }
                }
                give_order(u, x, y, o);
                set_stat(u, x, S_RETARGET_X1);
                set_stat(u, y, S_RETARGET_Y1);
                set_stat(u, o, S_RETARGET_ORDER);
            }
            if (ai_fixed)
            {
                byte o = *((byte *)((uintptr_t)u + S_OWNER));
                byte m = *((byte *)((uintptr_t)u + S_MANA));
                if ((*(byte *)(CONTROLER_TYPE + o) == C_COMP))
                {
                    if (m = 0x55) // 85 default starting mana
                    {
                        char buf[] = "\xA0"; // 160
                        PATCH_SET((char *)u + S_MANA, buf);
                    }
                }
            }
        }
    }
    return u;
}

PROC g_proc_00451728;
void unit_kill_deselect(int *u)
{
    int *ud = u;
    ((void (*)(int *))g_proc_00451728)(u); // original
    if (ai_fixed)
    {
        for (int i = 0; i < 16; i++)
        {
            int *p = (int *)(UNITS_LISTS + 4 * i);
            if (p)
            {
                p = (int *)(*p);
                while (p)
                {
                    byte id = *((byte *)((uintptr_t)p + S_ID));
                    bool f = ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER));
                    bool f2 = ((id == U_DWARWES) || (id == U_GOBLINS));
                    if (f)
                    {
                        if (!check_unit_dead(p) && check_unit_complete(p))
                        {
                            byte a1 = *((byte *)((uintptr_t)p + S_RETARGET_X1));
                            byte a2 = *((byte *)((uintptr_t)p + S_RETARGET_X1 + 1));
                            byte a3 = *((byte *)((uintptr_t)p + S_RETARGET_X1 + 2));
                            byte a4 = *((byte *)((uintptr_t)p + S_RETARGET_X1 + 3));
                            int *tr = (int *)(a1 + 256 * a2 + 256 * 256 * a3 + 256 * 256 * 256 * a4);
                            if (tr == ud)
                            {
                                set_stat(p, 0, S_RETARGET_X1);
                                set_stat(p, 0, S_RETARGET_X1 + 1);
                                set_stat(p, 0, S_RETARGET_X1 + 2);
                                set_stat(p, 0, S_RETARGET_X1 + 3);
                            }
                        }
                    }
                    if (f2)
                    {
                        if (!check_unit_dead(p))
                        {
                            byte a1 = *((byte *)((uintptr_t)p + S_ORDER_UNIT_POINTER));
                            byte a2 = *((byte *)((uintptr_t)p + S_ORDER_UNIT_POINTER + 1));
                            byte a3 = *((byte *)((uintptr_t)p + S_ORDER_UNIT_POINTER + 2));
                            byte a4 = *((byte *)((uintptr_t)p + S_ORDER_UNIT_POINTER + 3));
                            int *tr = (int *)(a1 + 256 * a2 + 256 * 256 * a3 + 256 * 256 * 256 * a4);
                            if (tr == ud)
                            {
                                set_stat(p, 0, S_ORDER_UNIT_POINTER);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 1);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 2);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 3);
                                give_order(p, 0, 0, ORDER_STOP);
                            }
                        }
                    }
                    p = (int *)(*((int *)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
    }

    WORD uid = unit_fixup(u);

    memset(build_queue + uid * 16, 255, 2 * 8);//clear build orders

    int kk = 0;
    while (kk < 1600)
    {
        int k = 0;
        while (k <= 15)
        {
            if (order_type_target(order_queue[kk * 48 + k * 3]))
            {
                if (uid == ((WORD)(order_queue[kk * 48 + k * 3 + 1] + 256 * order_queue[kk * 48 + k * 3 + 2])))
                {
                    //someone have this unit as order
                    while (k < 15)//move copy queue 
                    {
                        order_queue[kk * 48 + k * 3] = order_queue[kk * 48 + k * 3 + 3];
                        order_queue[kk * 48 + k * 3 + 1] = order_queue[kk * 48 + k * 3 + 4];
                        order_queue[kk * 48 + k * 3 + 2] = order_queue[kk * 48 + k * 3 + 5];
                        k++;
                    }
                    order_queue[kk * 48 + 15 * 3] = 255;//remove last
                    order_queue[kk * 48 + 15 * 3 + 1] = 255;
                    order_queue[kk * 48 + 15 * 3 + 2] = 255;
                }
            }
            k++;
        }
        kk++;
    }
}

void allow_table(byte p, int t, byte n, byte a)
{
    if (t == 0)
        t = ALLOWED_UNITS + (4 * p) + (n / 8);
    else if (t == 1)
        t = ALLOWED_UPGRADES + (4 * p) + (n / 8);
    else if (t == 2)
        t = ALLOWED_SPELLS + (4 * p) + (n / 8);
    else
        t = SPELLS_LEARNED + (4 * p) + (n / 8);
    byte b = *(byte *)t;
    if (a == 1)
        b |= (1 << (n % 8));
    else
        b &= (~(1 << (n % 8)));
    char buf[] = "\xff";
    buf[0] = b;
    PATCH_SET((char *)t, buf);
}

//---------------rocks
int *peon_rclick = NULL;

PROC g_proc_0043B888;
int rclick_get_unit_first()
{
    int original = ((int (*)())g_proc_0043B888)(); // original
    peon_rclick = (int *)original;
    return original;
}

PROC g_proc_0043B94B;
int rclick_get_unit_next()
{
    int original = ((int (*)())g_proc_0043B94B)(); // original
    peon_rclick = (int *)original;
    return original;
}

PROC g_proc_0043B779; // rclick
int rclick_tile_is_chopping_tree(int a)
{
    int original = ((int (*)(int))g_proc_0043B779)(a); // original
    if (peon_rclick != NULL)
    {
        byte pf = *((byte *)((uintptr_t)peon_rclick + S_PEON_FLAGS));
        if (original == 0)
        {
            char buf[] = "\xFF";
            PATCH_SET((char *)ROCKS_TILE_IS_CHOPPING_TREE, buf);
            original = ((int (*)(int))g_proc_0043B779)(a); // original
            buf[0] = '\xFC';
            PATCH_SET((char *)ROCKS_TILE_IS_CHOPPING_TREE, buf);
            if (original == 1)
                pf |= 1;
        }
        else
            pf &= ~1;
        set_stat(peon_rclick, pf, S_PEON_FLAGS);
    }
    return original;
}

PROC g_proc_0043B768;
int rclick_tile_is_tree(int a)
{
    int original = ((int (*)(int))g_proc_0043B768)(a); // original
    if (peon_rclick != NULL)
    {
        byte pf = *((byte *)((uintptr_t)peon_rclick + S_PEON_FLAGS));
        if (original == 0)
        {
            char buf[] = "\xFD";
            PATCH_SET((char *)ROCKS_TILE_IS_TREE, buf);
            original = ((int (*)(int))g_proc_0043B768)(a); // original
            buf[0] = '\xFE';
            PATCH_SET((char *)ROCKS_TILE_IS_TREE, buf);
            if (original == 1)
                pf |= 1;
        }
        else
            pf &= ~1;
        set_stat(peon_rclick, pf, S_PEON_FLAGS);
    }
    return original;
}

PROC g_proc_0042411E; // dispatch
int dispatch_tile_is_chopping_tree(int a)
{
    int original = ((int (*)(int))g_proc_0042411E)(a); // original
    if (original == 0)
    {
        char buf[] = "\xFF";
        PATCH_SET((char *)ROCKS_TILE_IS_CHOPPING_TREE, buf);
        original = ((int (*)(int))g_proc_0042411E)(a); // original
        buf[0] = '\xFC';
        PATCH_SET((char *)ROCKS_TILE_IS_CHOPPING_TREE, buf);
    }
    return original;
}

PROC g_proc_0042412B; // harvest dispatch
PROC g_proc_00451554; // unit kill
PROC g_proc_00453001; // unit check orders
PROC g_proc_0045307D; // unit set curr action
void dispatch_tile_cancel_tree_harvest(int *peon)
{
    byte id = *((byte *)((uintptr_t)peon + S_ID));
    if ((id == U_PEASANT) || (id == U_PEON))
    {
        byte pf = *((byte *)((uintptr_t)peon + S_PEON_FLAGS));
        if (pf & PEON_CHOPPING)
        {
            ((void (*)(int *))g_proc_0042412B)(peon); // original
            pf |= PEON_CHOPPING;                      // | PEON_SAVED_LOCATION;
            set_stat(peon, pf, S_PEON_FLAGS);
            char buf[] = "\xFF";
            PATCH_SET((char *)ROCKS_TILE_CANCEL, buf);
            buf[0] = '\xFD';
            PATCH_SET((char *)ROCKS_TILE_CANCEL2, buf);
            ((void (*)(int *))g_proc_0042412B)(peon); // original
            buf[0] = '\xFC';
            PATCH_SET((char *)ROCKS_TILE_CANCEL, buf);
            buf[0] = '\xFE';
            PATCH_SET((char *)ROCKS_TILE_CANCEL2, buf);
            // pf &= ~(PEON_CHOPPING | PEON_SAVED_LOCATION);
            // set_stat(peon, pf, S_PEON_FLAGS);
        }
        else
            ((void (*)(int *))g_proc_0042412B)(peon); // original
    }
    else
        ((void (*)(int *))g_proc_0042412B)(peon); // original
}

PROC g_proc_004241AD; // unit set curr action
void dispatch_tile_finish_tree_harvest(int *peon)
{
    ((void (*)(int *))g_proc_004241AD)(peon); // original
    byte x = *((byte *)((uintptr_t)peon + S_ORDER_X));
    byte y = *((byte *)((uintptr_t)peon + S_ORDER_Y));
    byte mxs = *(byte *)MAP_SIZE;                  // map size
    char *reg = (char *)*(int *)(MAP_REG_POINTER); // map reg
    char preg = *(char *)(reg + 2 * x + 2 * y * mxs);
    if (preg == '\xFF')
    {
        char buf[] = "\xFD";
        PATCH_SET((char *)(reg + 2 * x + 2 * y * mxs), buf);
        tile_remove_rocks(x, y);
        byte pf = *((byte *)((uintptr_t)peon + S_PEON_FLAGS));
        pf &= ~PEON_HARVEST_LUMBER;
        pf |= PEON_HARVEST_GOLD;
        set_stat(peon, pf, S_PEON_FLAGS);
    }
}

PROC g_proc_00424215;
int dispatch_tile_is_tree(int a)
{
    // peon_dispatch = (int*)(a - S_ORDER_X);
    int *peon_dispatch = (int *)*(int *)ROCKS_PEON_DISPATCH;
    int original = 0;
    if (peon_dispatch != NULL)
    {
        byte pf = *((byte *)((uintptr_t)peon_dispatch + S_PEON_FLAGS));
        original = ((int (*)(int))g_proc_00424215)(a); // original
        if (original == 0)
        {
            char buf[] = "\xFD";
            PATCH_SET((char *)ROCKS_TILE_IS_TREE, buf);
            original = ((int (*)(int))g_proc_00424215)(a); // original
            buf[0] = '\xFE';
            PATCH_SET((char *)ROCKS_TILE_IS_TREE, buf);
            if (original == 1)
            {
                buf[0] = '\xFF';
                PATCH_SET((char *)ROCKS_TILE_IS_TREE2, buf);
                pf |= 1;
            }
        }
        else
            pf &= ~1;
        set_stat(peon_dispatch, pf, S_PEON_FLAGS);
    }
    return original;
}

PROC g_proc_0042423D;
void dispatch_tile_start_tree_harvest(int *peon)
{
    ((void (*)(int *))g_proc_0042423D)(peon); // original
    byte ow = *((byte*)((uintptr_t)peon + S_OWNER));
    byte pf = *((byte *)((uintptr_t)peon + S_PEON_FLAGS));
    byte chp = 0;
    if (*(char *)ROCKS_TILE_START == '\xFF')
    {
        char buf[] = "\xFC";
        PATCH_SET((char *)ROCKS_TILE_START, buf);
        pf |= 1; // unused flag?
        if (ow == *(byte*)LOCAL_PLAYER)
        {
            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_RAISE)) != 0)chp += 20;
        }
    }
    else
    {
        pf &= ~1;
        if (ow == *(byte*)LOCAL_PLAYER)
        {
            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_RAISE)) != 0)chp += 20;
        }
    }
    set_stat(peon, chp, S_PEON_TREE_CHOPS);
    set_stat(peon, pf, S_PEON_FLAGS);
}

PROC g_proc_0042413C; // dispatch1
PROC g_proc_00424262; // dispatch2
PROC g_proc_004377EC; // dispatch3
int dispatch_tile_find_tree(int *a)
{
    int *peon_dispatch = (int *)*(int *)ROCKS_PEON_DISPATCH;
    int original = 0;
    if (peon_dispatch != NULL)
    {
        byte pf = *((byte *)((uintptr_t)peon_dispatch + S_PEON_FLAGS));
        if ((pf & 1) == 0)
            original = ((int (*)(int *))g_proc_0042413C)(a); // original
        else
        {
            char buf[] = "\xFD";
            PATCH_SET((char *)ROCKS_TILE_FIND_TREE, buf);
            original = ((int (*)(int *))g_proc_0042413C)(a); // original
            buf[0] = '\xFE';
            PATCH_SET((char *)ROCKS_TILE_FIND_TREE, buf);
        }
    }
    return original;
}

void finish_not_function()
{
    *(byte *)LOCAL_PLAYER = 0;
    *(byte *)PACKET_PLAYER = 1;
    *(byte *)PLAYER_HOST = 2;
    *(byte *)ME_HOST = 3;
}
//---------------rocks

#define SAVE_FILES_NAMES 0x004B50A4
char save_named[MAX_PATH];
char my_dir[MAX_PATH];
char autosave_named[MAX_PATH];
char autosave_name[MAX_PATH];

void sfile_read_set_default()
{
    //set default data
    memset(build_queue, 255, sizeof(build_queue));
    memset(order_queue, 255, sizeof(order_queue));
    memset(names_secret, 0, sizeof(names_secret));
}

void sfile_read_custom()
{
    sfile_read_set_default();
    FILE* fp;
    if ((fp = fopen(save_named, "rb")) != NULL)
    {
        byte used = 0;
        fseek(fp, 50, 0);
        fread(&used, 1, 1, fp);
        if (used != 255)
        {
            fseek(fp, 2, 0);
            DWORD orig_size = 0;
            fread(&orig_size, 4, 1, fp);
            fseek(fp, orig_size, 0);
            while (!feof(fp))
            {
                DWORD section_name = 0;
                DWORD section_size = 0;
                fread(&section_name, 4, 1, fp);
                fread(&section_size, 4, 1, fp);
                if (section_name == BUILD_QUEUE_SECTION)//BQBQ
                {
                    fread(&build_queue, 1, sizeof(build_queue), fp);
                }
                else if (section_name == ORDER_QUEUE_SECTION)//OQOQ
                {
                    fread(&order_queue, 1, sizeof(order_queue), fp);
                }
                else if (section_name == RMPQ_SECTION)//RMPQ
                {
                    fread(&names_secret, 1, sizeof(names_secret), fp);
                }
                else
                {
                    if (section_size == 0)section_size = 1;
                    fseek(fp, section_size - 1, SEEK_CUR);//skip unk section
                    byte load_b = 0;
                    fread(&load_b, 1, 1, fp);
                }
            }
        }
        fclose(fp);
    }
}

PROC g_proc_0043C5F5;
int sfile_read(byte slot)
{
    int original = ((int (*)(byte))g_proc_0043C5F5)(slot);//original

    if (original)
    {
        unsigned char* save_name = (unsigned char*)(SAVE_FILES_NAMES + (slot * MAX_PATH));
        if (save_name)
        {
            for (int i = 0; i < MAX_PATH; i++)save_named[i] = save_name[i];
            sfile_read_custom();
        }
    }

    return original;
}

void sfile_write_custom(byte slot)
{
    unsigned char* save_name = (unsigned char*)(SAVE_FILES_NAMES + (slot * MAX_PATH));
    if (save_name)
    {
        for (int i = 0; i < MAX_PATH; i++)save_named[i] = save_name[i];
        FILE* fp;
        if ((fp = fopen(save_named, "r+b")) != NULL)
        {
            bool write_bqbq = true;
            bool write_oqoq = true;

            byte used = 0;
            fseek(fp, 50, 0);
            fread(&used, 1, 1, fp);
            if (used == 255)
            {
                fseek(fp, 0, SEEK_END);
                DWORD orig_size = ftell(fp);
                WORD start = 0x1a31;
                fseek(fp, 0, 0);
                fwrite(&start, 2, 1, fp);
                fwrite(&orig_size, 4, 1, fp);
                fseek(fp, 50, 0);
                used = 1;
                fwrite(&used, 1, 1, fp);
                fseek(fp, orig_size, 0);
            }
            else
            {
                fseek(fp, 2, 0);
                DWORD orig_size = 0;
                fread(&orig_size, 4, 1, fp);
                fseek(fp, orig_size, 0);
                while (!feof(fp))
                {
                    DWORD section_name = 0;
                    DWORD section_size = 0;
                    fread(&section_name, 4, 1, fp);
                    fread(&section_size, 4, 1, fp);
                    if (section_name == BUILD_QUEUE_SECTION)//BQBQ
                    {
                        fseek(fp, section_size, SEEK_CUR);
                        write_bqbq = false;//already have
                    }
                    else if (section_name == ORDER_QUEUE_SECTION)//OQOQ
                    {
                        fseek(fp, section_size, SEEK_CUR);
                        write_oqoq = false;//already have
                    }
                    else
                    {
                        if (section_size == 0)section_size = 1;
                        fseek(fp, section_size - 1, SEEK_CUR);//skip unk section
                        byte load_b = 0;
                        fread(&load_b, 1, 1, fp);
                    }
                }
            }

            if (write_bqbq)
            {
                DWORD section_name = BUILD_QUEUE_SECTION;
                fwrite(&section_name, 4, 1, fp);
                DWORD section_size = sizeof(build_queue);
                fwrite(&section_size, 4, 1, fp);
                fwrite(&build_queue, 1, sizeof(build_queue), fp);
            }

            if (write_oqoq)
            {
                DWORD section_name2 = ORDER_QUEUE_SECTION;
                fwrite(&section_name2, 4, 1, fp);
                DWORD section_size2 = sizeof(order_queue);
                fwrite(&section_size2, 4, 1, fp);
                fwrite(&order_queue, 1, sizeof(order_queue), fp);
            }

            DWORD section_name3 = RMPQ_SECTION;
            fwrite(&section_name3, 4, 1, fp);
            DWORD section_size3 = 2;
            fwrite(&section_size3, 4, 1, fp);
            fwrite(&names_secret, 1, sizeof(names_secret), fp);

            fclose(fp);
        }
    }
}

PROC g_proc_0043D528;//for new file
PROC g_proc_0043CFF2;//for exist file
int sfile_write(byte slot, int name)
{
    int original = ((int (*)(byte, int))g_proc_0043D528)(slot, name);//original

    if (original)
    {
        sfile_write_custom(slot);
    }

    return original;
}

DWORD autosave_counter;

int sfile_write_auto()
{
    for (int i = 0; i < MAX_PATH; i++)*(char*)(SAVE_FILES_NAMES + i) = autosave_named[i];
    //int original = ((int (*)(byte, char*))0x0043EAB0)(0, autosave_name);
    return ((int (*)(byte, char*))(PROC)((char*)0x0043D528 + 5 + *((DWORD*)(&((char*)0x0043D528)[1]))))(0, autosave_name);
}

void bldg_build_add_queue(int* u, byte id, byte o)
{
    WORD uid = unit_fixup(u);
    int k = 0;
    while (k <= 7)
    {
        if (build_queue[uid * 16 + k * 2] == 255)
        {
            build_queue[uid * 16 + k * 2] = id;
            build_queue[uid * 16 + k * 2 + 1] = o;
            break;
        }
        k++;
    }
}

PROC g_proc_00455F9C;//single
PROC g_proc_0047632D;//bne mp
int bldg_build_start_packet(int* u, byte id, byte o)
{
    int original = ((int (*)(int*, byte, byte))g_proc_00455F9C)(u, id, o);//original

    if (original == 0)
    {
        if ((*((byte*)((uintptr_t)u + S_FLAGS1)) & UF_BUILD_ON) != 0)
        {
            bldg_build_add_queue(u, id, o);
        }
    }

    return original;
}

byte get_order_by_function(DWORD f)
{
    int k = 0;
    while (k < ORDER_NONE)
    {
        if ((*(DWORD*)(ORDER_FUNCTIONS + 4 * k)) == f)return k;
        k++;
    }
    if (f == 0x00436A80)return ORDER_CREATE_BLDG;
    return ORDER_STOP;
}

byte global_bid = 0;
byte global_bx = 0;
byte global_by = 0;

void unit_set_target_add_queue(int* u, WORD x, WORD y, int* t, byte o)
{
    byte ow = *((byte*)((uintptr_t)u + S_OWNER));
    if ((*(byte*)(CONTROLER_TYPE + ow) == C_PLAYER))
    {
        WORD uid = unit_fixup(u);
        WORD tid = 65535;
        if (t != NULL)
        {
            tid = unit_fixup(t);
            if (o == ORDER_ATTACK)o = ORDER_ATTACK_TARGET;
            if (o == ORDER_MOVE)o = ORDER_FOLLOW;
            if (o == ORDER_DEMOLISH)o = ORDER_DEMOLISH_NEAR;
        }
        else
        {
            tid = x + y * 256;
            if (o == ORDER_ATTACK)o = ORDER_ATTACK_AREA;
            if (o == ORDER_DEMOLISH)o = ORDER_DEMOLISH_AT;

            if (o == ORDER_CREATE_BLDG)
            {
                o = *((byte*)((uintptr_t)u + S_PEON_BUILD)) + ORDER_NONE + 1;
                tid = *((byte*)((uintptr_t)u + S_PEON_BUILD_X)) + *((byte*)((uintptr_t)u + S_PEON_BUILD_Y)) * 256;

                byte od = *((byte*)((uintptr_t)u + S_ORDER));
                if (od == ORDER_CREATE_BLDG)
                {
                    set_stat(u, global_bid, S_PEON_BUILD);
                    set_stat(u, global_bx, S_PEON_BUILD_X);
                    set_stat(u, global_by, S_PEON_BUILD_Y);
                }
            }
        }
        int k = 0;
        while (k <= 15)
        {
            if (order_queue[uid * 48 + k * 3] == 255)
            {
                order_queue[uid * 48 + k * 3] = o;
                order_queue[uid * 48 + k * 3 + 1] = tid % 256;
                order_queue[uid * 48 + k * 3 + 2] = tid / 256;
                break;
            }
            k++;
        }
    }
}

PROC g_proc_00455EA2;//action
PROC g_proc_0043B943_2;//rclick
PROC g_proc_0043B116;//placebox
void unit_set_target(int* u, WORD x, WORD y, int* t, DWORD o)
{
    byte id = *(byte*)((uintptr_t)u + S_ID);
    if (id < U_FARM)
    {
        if (check_key_pressed(KEY_ID_SHIFT))unit_set_target_add_queue(u, x, y, t, get_order_by_function(o));
        else
        {
            //clear queue
            WORD uid = unit_fixup(u);
            int k = 0;
            while (k <= 15)
            {
                order_queue[uid * 48 + k * 3] = 255;
                order_queue[uid * 48 + k * 3 + 1] = 255;
                order_queue[uid * 48 + k * 3 + 2] = 255;
                k++;
            }

            ((void (*)(int*, WORD, WORD, int*, DWORD))g_proc_00455EA2)(u, x, y, t, o);//original
        }
    }
    else ((void (*)(int*, WORD, WORD, int*, DWORD))g_proc_00455EA2)(u, x, y, t, o);//original
}
void unit_set_target2(int* u, WORD x, WORD y, int* t, DWORD o)
{
    byte id = *(byte*)((uintptr_t)u + S_ID);
    if (id < U_FARM)
    {
        if (check_key_pressed(KEY_ID_SHIFT))unit_set_target_add_queue(u, x, y, t, get_order_by_function(o));
        else
        {
            //clear queue
            WORD uid = unit_fixup(u);
            int k = 0;
            while (k <= 15)
            {
                order_queue[uid * 48 + k * 3] = 255;
                order_queue[uid * 48 + k * 3 + 1] = 255;
                order_queue[uid * 48 + k * 3 + 2] = 255;
                k++;
            }

            ((void (*)(int*, WORD, WORD, int*, DWORD))g_proc_0043B943_2)(u, x, y, t, o);//original
        }
    }
    else ((void (*)(int*, WORD, WORD, int*, DWORD))g_proc_0043B943_2)(u, x, y, t, o);//original
}

PROC g_proc_0043B0EF;//placebox
void find_bldg_entry_corner(int* u, DWORD xy, byte bid)
{
    ((void (*)(int*, DWORD, byte))g_proc_0043B0EF)(u, xy, bid);//original
    byte od = *((byte*)((uintptr_t)u + S_ORDER));
    if (od == ORDER_CREATE_BLDG)
    {
        global_bid = *((byte*)((uintptr_t)u + S_PEON_BUILD));
        global_bx = *((byte*)((uintptr_t)u + S_PEON_BUILD_X));
        global_by = *((byte*)((uintptr_t)u + S_PEON_BUILD_Y));
    }
}

void send_cheat(byte c)
{
    int b = *(int *)CHEATBITS;
    if ((b & (1 << c)) != 0)
        b &= ~(1 << c);
    else
        b |= 1 << c;
    ((void (*)(int))F_SEND_CHEAT_PACKET)(b);
}

void rec_autoheal()
{
    byte p = *(byte *)PACKET_PLAYER; // player
    byte local = *(byte *)LOCAL_PLAYER;
    byte b = *(byte *)(SPELLS_LEARNED + 4 * p);
    byte sp = GREATER_HEAL;
    if ((b & (1 << sp)) != 0)
        b &= ~(1 << sp);
    else
        b |= 1 << sp;
    char buf[] = "\x0";
    buf[0] = b;
    PATCH_SET((char *)(SPELLS_LEARNED + 4 * p), buf);

    if ((*(byte *)(SPELLS_LEARNED + 4 * (*(byte *)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
    {
        churc[20 * 3 + 2] = '\x5b'; // icon
        if (p == local)
        {
            char msg[] = "autoheal\x5 enabled";
            show_message(10, msg);
        }
    }
    else
    {
        churc[20 * 3 + 2] = '\x6d'; // icon
        if (p == local)
        {
            char msg[] = "autoheal\x3 disabled";
            show_message(10, msg);
        }
    }
    ((void (*)())F_STATUS_REDRAW)();
}

PROC g_proc_0045614E;
void receive_cheat_single(int c, int a1)
{
    char buf[] = "\x0";
    buf[0] = *(byte *)LOCAL_PLAYER;
    PATCH_SET((char *)PACKET_PLAYER, buf);
    bool f = true;
    if ((c & (1 << 8)) != 0) // 8 - autoheal
    {
        rec_autoheal();
        f = false;
    }
    if ((c & (1 << 9)) != 0) // 9 - attack peons
    {
        // rec_peons();
        f = false;
    }
    if (f)
    {
        c &= 141439; // 0 on screen;1 fast build;2 god;3 magic;4 upgrades;5 money;6 finale;11 hatchet;17 disco;13 tigerlily
        if (*(byte *)LEVEL_OBJ == LVL_ORC1)
            c = 0;
        ((void (*)(int, int))g_proc_0045614E)(c, a1); // orig
    }
    else
    {
        char buf[] = "\x0";
        PATCH_SET((char *)PLAYER_CHEATED, buf);
    }
}

void button_autoheal(int)
{
    send_cheat(8);
    if ((*(byte *)(SPELLS_LEARNED + 4 * (*(byte *)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
        churc[20 * 3 + 2] = '\x5b'; // icon
    else
        churc[20 * 3 + 2] = '\x6d'; // icon
    ((void (*)())F_STATUS_REDRAW)();
}

void autoheal(bool b)
{
    if (b)
    {
        if ((*(byte *)(SPELLS_LEARNED + 4 * (*(byte *)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
            churc[20 * 3 + 2] = '\x5b'; // icon
        else
            churc[20 * 3 + 2] = '\x6d'; // icon

        void (*r1)(int) = button_autoheal;
        patch_setdword((DWORD *)(churc + (20 * 3 + 8)), (DWORD)r1);

        char b1[] = "\04\x0\x0\x0\x68\x37\x4a\x0";
        char *repf = churc;
        patch_setdword((DWORD *)(b1 + 4), (DWORD)repf);
        //PATCH_SET((char *)CHURCH_BUTTONS, b1);
        A_autoheal = true;
    }
    else
    {
        char b1[] = "\03\x0\x0\x0\x68\x37\x4a\x0";
        //PATCH_SET((char *)CHURCH_BUTTONS, b1);
        A_autoheal = false;
    }
}

void call_default_kill() // default kill all victory
{
    byte l = *(byte *)LOCAL_PLAYER;
    if (!slot_alive(l))
        lose(true);
    else
    {
        if (!check_opponents(l))
            win(true);
    }
}

PROC g_proc_0043789D;
void unit_next_action(int* u, byte o)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if ((id == U_PEON) || (id == U_PEASANT))
    {
        byte od = *((byte*)((uintptr_t)u + S_ORDER));
        if (od == ORDER_CREATE_BLDG)return;
    }
    ((void (*)(int*, byte))g_proc_0043789D)(u, o);//original
}
//-------------files
const char FILES_PATH[] = ".\\RedMistPrequel\\";

void grp_convert(DWORD grp, WORD frame, byte* arr, int w, int h)
{
    for (int i = 0; i < w; i++)for (int j = 0; j < h; j++)arr[i + j * w] = 0;//clear data

    if (grp != 0)
    {
        int pnt = 0;
        WORD count = *(WORD*)(grp + pnt); pnt += 2;
        WORD size_x = *(WORD*)(grp + pnt); pnt += 2;
        WORD size_y = *(WORD*)(grp + pnt); pnt += 2;

        for (int i = 0; i < frame; i++)pnt += 8;//skip to needed frame directly

        byte ofx = *(byte*)(grp + pnt); pnt += 1;
        byte ofy = *(byte*)(grp + pnt); pnt += 1;
        byte line_w = *(byte*)(grp + pnt); pnt += 1;
        byte lines = *(byte*)(grp + pnt); pnt += 1;
        if (lines > h)lines = h % 255;

        DWORD data_of = *(DWORD*)(grp + pnt);

        byte cline = 0;
        while (cline < lines)
        {
            pnt = data_of + 2 * cline;
            WORD data_s = *(WORD*)(grp + pnt);
            pnt = data_of + data_s;

            byte cpos = 0;
            while (cpos < line_w)
            {
                byte codeb = *(byte*)(grp + pnt); pnt += 1;
                if (codeb > 0x80)cpos += (codeb - 0x80);//shift position
                else if (codeb > 0x40)
                {
                    codeb -= 0x40;
                    byte pix = *(byte*)(grp + pnt); pnt += 1;
                    while (codeb > 0)//repeat 1 pixel many times
                    {
                        arr[ofx + w * ofy + cline * w + cpos] = pix; cpos++;
                        codeb--;
                    }
                }
                else
                {
                    while (codeb > 0)//pixels data
                    {
                        byte pix = *(byte*)(grp + pnt); pnt += 1;
                        arr[ofx + w * ofy + cline * w + cpos] = pix; cpos++;
                        codeb--;
                    }
                }
            }
            cline++;
        }
    }
}

void grp_player_color(byte* arr, int w, int h, byte col)
{
    if (col != 0)
    {
        for (int i = 0; i < w; i++)
        {
            for (int j = 0; j < h; j++)
            {
                if ((arr[i + j * w] >= 208) && (arr[i + j * w] <= 211))//red player
                {
                    if (col < 7)arr[i + j * w] += 4 * col;
                    else if (col == 7)arr[i + j * w] -= 196;
                }
            }
        }
    }
}

void grp_convert_player_color(DWORD grp, WORD frame, byte* arr, int w, int h)
{
    grp_convert(grp, frame, arr, w, h);
    grp_player_color(arr, w, h, *(byte*)(PLAYERS_COLORS + *(byte*)LOCAL_PLAYER));
}

void* tbl_stats;

void* grp_rally_flag;
void* grp_rank;

DWORD global_grp_port = 0;

PROC g_proc_late_004453A7 = NULL;//draw unit port
void grp_draw_portrait2(void* grp, byte frame, int b, int c)
{
    global_grp_port = (DWORD)grp;
    ((void (*)(void*, byte, int, int))g_proc_late_004453A7)(grp, frame, b, c);//original
}

byte spr_portrait_big[46 * 38];
byte spr_portrait_small[23 * 19];

void grp_make_small_port(DWORD grp, WORD frame)
{
    if (grp != 0)
    {
        grp_convert_player_color(grp, frame, spr_portrait_big, 46, 38);
        for (int i = 0; i < 23; i++)
        {
            for (int j = 0; j < 19; j++)
            {
                spr_portrait_small[i + j * 23] = spr_portrait_big[i * 2 + j * 46 * 2];//50% size
            }
        }
    }
}

byte spr_flag[32 * 32];

void grp_make_rally_flag(byte frame)
{
    grp_convert_player_color((DWORD)grp_rally_flag, frame, spr_flag, 32, 32);
}

byte spr_rank[28 * 14];

void grp_make_rank(byte frame)
{
    grp_convert((DWORD)grp_rank, frame, spr_rank, 28, 14);
}

byte spr_portrait_empty[23 * 19] = {
239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
239,239,239,239,239,239,239,163,166,167,167,167,167,167,166,163,239,239,239,239,239,239,239,
239,239,239,239,239,239,167,166,165,165,164,164,164,165,165,165,165,162,239,239,239,239,239,
239,239,239,239,239,167,165,164,162,239,239,239,239,239,165,166,165,165,163,239,239,239,239,
239,239,239,239,167,165,164,239,239,239,239,239,239,239,167,165,165,165,165,239,239,239,239,
239,239,239,163,166,165,239,239,239,239,239,239,239,167,165,165,163,164,165,163,239,239,239,
239,239,239,165,166,163,239,239,239,239,239,239,167,165,165,163,239,162,165,165,239,239,239,
239,239,239,167,165,239,239,239,239,239,239,167,165,165,163,239,239,239,165,165,162,239,239,
239,239,239,168,165,239,239,239,239,239,167,165,165,163,239,239,239,239,165,165,162,239,239,
239,239,239,168,164,239,239,239,239,168,165,165,163,239,239,239,239,239,165,165,162,239,239,
239,239,239,167,165,239,239,239,168,165,165,163,239,239,239,239,239,239,165,165,162,239,239,
239,239,239,165,166,162,239,168,165,165,163,239,239,239,239,239,239,163,166,165,239,239,239,
239,239,239,163,166,165,168,165,165,163,239,239,239,239,239,239,239,165,165,163,239,239,239,
239,239,239,239,165,166,165,165,163,239,239,239,239,239,239,239,165,165,165,239,239,239,239,
239,239,239,239,163,165,165,165,163,239,239,239,239,239,163,165,165,165,163,239,239,239,239,
239,239,239,239,239,162,164,165,165,165,166,166,166,166,166,165,165,163,239,239,239,239,239,
239,239,239,239,239,239,239,163,164,165,165,165,165,165,164,163,239,239,239,239,239,239,239,
239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
};


//------------------------------
char grp_name[MAX_PATH];
void* grps_def[U_DEAD + 4];
void* grps_winter[U_DEAD + 4];
void* grps_xswamp[U_DEAD + 4];

void *grp_port_forest;
void *grp_port_winter;
void *grp_port_wast;
void *grp_port_swamp;
void *grp_saw;
void* grp_shield;
void* grp_farm;
void* grp_regen;
void* grp_chop;
void* grp_instr;
void* grp_alebard;
void* grp_leg;
void* grp_bash;
void* grp_beer;

void *grp_dwarf_dead;

void* grp_bolt;

void *tbl_credits;
char tbl_credits_title[] = "THE MIST TURNS RED             PREQUEL ACT 1";

void *tbl_brif1;
void *tbl_brif2;
void *tbl_brif3;
void *tbl_brif4;
void *tbl_brif5;
void *tbl_brif_secret;
void *tbl_end;
void* tbl_end2;
char tbl_brif8[] = " ";

void *tbl_task1;
void *tbl_task2;
void *tbl_task3;
void *tbl_task4;
void *tbl_task5;
void *tbl_task_secret;

void *tbl_title1;
void *tbl_title2;
void *tbl_title3;
void *tbl_title4;
void *tbl_title5;
void *tbl_title_secret;

void *tbl_name1;
void *tbl_name2;
void *tbl_name3;
void *tbl_name4;
void *tbl_name8;
void *tbl_name9;
void *tbl_name10;
void *tbl_name11;
void* tbl_name12;
void* tbl_name13;
void* tbl_name14;
void* tbl_name15;
void* tbl_name16;
void* tbl_name17;
void* tbl_name18;
void* tbl_name19;
void* tbl_name20;
void* tbl_name21;
void* tbl_name22;
void* tbl_name23;
void* tbl_name24;
void* tbl_name25;
void* tbl_name26;
void* tbl_name27;
void* tbl_name28;
void* tbl_name29;
void* tbl_name30;
void* tbl_name31;
void* tbl_name32;
void* tbl_name33;
void* tbl_name34;
void* tbl_name35;

void* tbl_names_secret1;
void* tbl_names_secret2;
void* tbl_names_secret3;

bool hero_name_loaded = false;
char hero_name[MAX_PATH];
char hero_full_name[MAX_PATH];

void* tbl_skill1;
void *tbl_skill2;
void *tbl_skill3;
void* tbl_skill4;

void* tbl_research1;
void* tbl_research2;
void* tbl_research3;
void* tbl_research4;
void* tbl_research5;
void* tbl_research6;
void* tbl_research7;
void *tbl_research8;
void* tbl_research9;
void* tbl_research10;
void* tbl_research11;

void* tbl_train1;
void *tbl_train2;
void* tbl_train3;
void* tbl_train4;
void* tbl_train5;
void* tbl_train6;

void* tbl_build1;
void* tbl_build2;
void* tbl_build3;
void* tbl_build4;
void* tbl_build5;

void *tbl_nations;

void* tbl_error1;

void* tbl_reinforcements;

void *pud_map1;
DWORD pud_map1_size;
void *pud_map2;
DWORD pud_map2_size;
void *pud_map3;
DWORD pud_map3_size;
void *pud_map4;
DWORD pud_map4_size;
void *pud_map5;
DWORD pud_map5_size;
void *pud_emap1;
DWORD pud_emap1_size;
void *pud_emap2;
DWORD pud_emap2_size;
void *pud_emap3;
DWORD pud_emap3_size;
void *pud_emap4;
DWORD pud_emap4_size;
void *pud_emap5;
DWORD pud_emap5_size;
void* pud_map_secret;
DWORD pud_map_secret_size;

void *bin_menu;
DWORD bin_menu_size;
void *bin_menu_copy;
void *bin_sngl;
DWORD bin_sngl_size;
void *bin_sngl_copy;
void *bin_newcmp;
DWORD bin_newcmp_size;
void *bin_newcmp_copy;
void* bin_newcmp2;
DWORD bin_newcmp2_size;
void* bin_newcmp2_copy;
void *bin_quit;
DWORD bin_quit_size;
void *bin_quit_copy;
void* bin_hdisptch;
DWORD bin_hdisptch_size;
void* bin_hdisptch_copy;
void* bin_credits;
DWORD bin_credits_size;
void* bin_credits_copy;

void *bin_AI;
DWORD bin_AI_size;
void *bin_script;
DWORD bin_script_size;
void *bin_unitdata;
DWORD bin_unitdata_size;
void *bin_unitdato;
DWORD bin_unitdato_size;
void *bin_upgrades;
DWORD bin_upgrades_size;

void *pcx_end;
void *pcx_end_pal;
void* pcx_end2;
void* pcx_end2_pal;
void *pcx_credits;
void *pcx_credits_pal;

void *pcx_act1;
void *pcx_act1_pal;
void *pcx_act2;
void *pcx_act2_pal;
void *pcx_act3;
void *pcx_act3_pal;
void *pcx_act4;
void *pcx_act4_pal;
void *pcx_act5;
void *pcx_act5_pal;
void *pcx_act_secret;
void *pcx_act_secret_pal;

void *pcx_brif1;
void *pcx_brif1_pal;
void *pcx_brif2;
void *pcx_brif2_pal;
void *pcx_brif3;
void *pcx_brif3_pal;
void *pcx_brif4;
void *pcx_brif4_pal;
void *pcx_brif5;
void *pcx_brif5_pal;
void *pcx_brif_secret;
void *pcx_brif_secret_pal;

void* pcx_win;
void* pcx_win_pal;
void* pcx_loss;
void* pcx_loss_pal;

void* tile_iceland_cv4;
DWORD tile_iceland_cv4_size;
void* tile_iceland_vr4;
DWORD tile_iceland_vr4_size;
void* tile_iceland_vx4;
DWORD tile_iceland_vx4_size;
void* tile_iceland_palette;

void* tile_desert_cv4;
DWORD tile_desert_cv4_size;
void* tile_desert_vr4;
DWORD tile_desert_vr4_size;
void* tile_desert_vx4;
DWORD tile_desert_vx4_size;
void* tile_desert_palette;

char* text_from_tbl(void* tbl, int str_id)
{
    return ((char* (*)(void*, int))F_GET_LINE_FROM_TBL)(tbl, str_id + 1);
}

PROC g_proc_004542FB;
int grp_draw_cross(int a, int *u, void *grp, int frame)
{
    void *new_grp = NULL;
    //-------------------------------------------------
    // if level = orc1
    // if race = human
    // etc
    // new_grp = grp_runecircle;

    if (new_grp)
        return ((int (*)(int, int *, void *, int))g_proc_004542FB)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int *, void *, int))g_proc_004542FB)(a, u, grp, frame); // original
}

PROC g_proc_00454DB4;
int grp_draw_bullet(int a, int *u, void *grp, int frame)
{
    bool draw = true;
    void *new_grp = NULL;
    //-------------------------------------------------
    // 44 - target
    byte id = *((byte *)((uintptr_t)u + 52));      // 52 - id
    int *c = (int *)*((int *)((uintptr_t)u + 48)); // 48 - creator
    if (id == B_BOLT)
    {
        byte cid = *((byte *)((uintptr_t)c + S_ID));
        if (cid == U_BALLISTA)new_grp = grp_bolt;
    }

    if (draw)
    {
        if (new_grp)
            return ((int (*)(int, int *, void *, int))g_proc_00454DB4)(a, u, new_grp, frame);
        else
            return ((int (*)(int, int *, void *, int))g_proc_00454DB4)(a, u, grp, frame); // original
    }
    return 0;
}

PROC g_proc_00454BCA;
int grp_draw_unit(int a, int *u, void *grp, int frame)
{
    void *new_grp = NULL;
    //-------------------------------------------------
    byte id = *((byte*)((uintptr_t)u + S_ID));
    byte spr_id = *((byte*)((uintptr_t)u + S_SPRITE));
    if (id == U_DEAD) // dead
    {
        byte m = *((byte *)((uintptr_t)u + S_MOVEMENT_TYPE));
        if (m == 100)
            new_grp = grp_dwarf_dead;
    }

    void** grpsp = grps_def;
    byte era = *(byte*)MAP_ERA;
    if (era == 1)grpsp = grps_winter;
    else if (era == 3)grpsp = grps_xswamp;
    if (id < U_DEAD)new_grp = grpsp[spr_id];
    if (id == U_PEASANT)
    {
        if (check_peon_loaded(u, 0))new_grp = grpsp[106];
        else if (check_peon_loaded(u, 1))new_grp = grpsp[105];
        else new_grp = grpsp[U_PEASANT];
    }
    else if (id == U_PEON)
    {
        if (check_peon_loaded(u, 0))new_grp = grpsp[108];
        else if (check_peon_loaded(u, 1))new_grp = grpsp[107];
        else new_grp = grpsp[U_PEON];
    }

    //not change dnarea
    if (*((byte*)((uintptr_t)u + S_OWNER)) == P_NEUTRAL)if ((id == U_KARGATH) || (id == U_GROM) || (id == U_CHOGAL) || (id == U_DENTARG))new_grp = NULL;

    if (new_grp)
        return ((int (*)(int, int *, void *, int))g_proc_00454BCA)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int *, void *, int))g_proc_00454BCA)(a, u, grp, frame); // original
}

PROC g_proc_00455599;
int grp_draw_building(int a, int *u, void *grp, int frame)
{
    void *new_grp = NULL;
    //-------------------------------------------------
    byte id = *((byte *)((uintptr_t)u + S_ID));
    byte spr_id = *((byte*)((uintptr_t)u + S_SPRITE));
    byte mp = *((byte *)((uintptr_t)u + S_MANA));
    WORD hp = *((WORD *)((uintptr_t)u + S_HP));
    WORD mhp = *(WORD *)(UNIT_HP_TABLE + 2 * id); // max hp
    if ((mp > 127) || check_unit_complete(u) || (hp >= (mhp / 2)))
    {
        new_grp = grps_def[id];
        byte era = *(byte*)MAP_ERA;
        if (era == 1)new_grp = grps_winter[spr_id];
        else if (era == 3)new_grp = grps_xswamp[spr_id];
        frame = check_unit_complete(u) ? 0 : 1;

        byte ord = *((byte*)((uintptr_t)u + S_ORDER));
        byte bord = *((byte*)((uintptr_t)u + S_BUILD_ORDER));
        if ((ord == ORDER_BLDG_BUILD) && (bord == 3))
        {
            if (id == U_HTOWER)
            {
                byte bt = *((byte*)((uintptr_t)u + S_BUILD_TYPE));
                if (bt == U_HARROWTOWER)
                {
                    new_grp = grps_def[U_HARROWTOWER];
                    if (era == 1)new_grp = grps_winter[U_HARROWTOWER];
                    if (era == 3)new_grp = grps_xswamp[U_HARROWTOWER];
                    frame = 1;
                }
                else if (bt == U_HCANONTOWER)
                {
                    new_grp = grps_def[U_HCANONTOWER];
                    if (era == 1)new_grp = grps_winter[U_HCANONTOWER];
                    if (era == 3)new_grp = grps_xswamp[U_HCANONTOWER];
                    frame = 1;
                }
            }
            else if (id == U_TOWN_HALL)
            {
                byte bt = *((byte*)((uintptr_t)u + S_BUILD_TYPE));
                if (bt == U_KEEP)
                {
                    new_grp = grps_def[U_KEEP];
                    if (era == 1)new_grp = grps_winter[U_KEEP];
                    if (era == 3)new_grp = grps_xswamp[U_KEEP];
                    frame = 1;
                }
            }
        }

        if (check_unit_complete(u))
        {
            if (id == U_MINE)
            {
                if (*((byte*)((uintptr_t)u + S_RESOURCES - 1)) != 0)frame = 1;
            }
            if (new_grp)
            {
                WORD mxf = *(WORD*)new_grp - 1;
                if (mxf >= 2)
                {
                    if ((ord == ORDER_BLDG_BUILD) && (bord != 3))
                    {
                        byte fr = *((byte*)((uintptr_t)u + S_BUILD_PROGRES_TOTAL + 2));
                        frame = fr;
                        byte ant = *((byte*)((uintptr_t)u + S_BUILD_PROGRES_TOTAL + 3));
                        if (ant == 0)
                        {
                            fr += 1;
                            if ((fr < 2) || (fr > mxf))fr = 2;
                            ant = 3;
                        }
                        else
                        {
                            ant--;
                            if (ant < 0)ant = 0;
                        }
                        set_stat(u, ant, S_BUILD_PROGRES_TOTAL + 3);
                        set_stat(u, fr, S_BUILD_PROGRES_TOTAL + 2);
                    }
                    else set_stat(u, 0, S_BUILD_PROGRES_TOTAL + 2);
                }
            }
        }
    }
    //-------------------------------------------------
    if (new_grp)
        return ((int (*)(int, int *, void *, int))g_proc_00455599)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int *, void *, int))g_proc_00455599)(a, u, grp, frame); // original
}

PROC g_proc_0043AE54;
void grp_draw_building_placebox(void *grp, int frame, int a, int b)
{
    void *new_grp = NULL;
    int *peon = (int *)*(int *)LOCAL_UNITS_SELECTED;
    byte id = (*(int *)b) % 256;
    //-------------------------------------------------
    if (id < U_DEAD)
    {
        new_grp = grps_def[id];
        byte era = *(byte*)MAP_ERA;
        if (era == 1)new_grp = grps_winter[id];
        else if (era == 3)new_grp = grps_xswamp[id];
    }
    //-------------------------------------------------
    if (new_grp)
        ((void (*)(void *, int, int, int))g_proc_0043AE54)(new_grp, frame, a, b);
    else
        ((void (*)(void *, int, int, int))g_proc_0043AE54)(grp, frame, a, b); // original
}

int *portrait_unit;

PROC g_proc_0044538D;
void grp_portrait_init(int *a)
{
    ((void (*)(int *))g_proc_0044538D)(a); // original
    portrait_unit = (int *)*((int *)((uintptr_t)a + 0x26));
}

PROC g_proc_004453A7; // draw unit port
void grp_draw_portrait(void *grp, byte frame, int b, int c)
{
    bool f = true;
    void *new_grp = NULL;
    //-------------------------------------------------
    int *u = portrait_unit;
    if (u != NULL)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        /*if (id == U_GRIFON)
        {
            new_grp = grp_ace_black_icon;
            frame = 1;
            f = false;
        }*/

        //not change dnarea
        if (*((byte*)((uintptr_t)u + S_OWNER)) == P_NEUTRAL)if ((id == U_KARGATH) || (id == U_GROM) || (id == U_CHOGAL) || (id == U_DENTARG))f = false;
    }

    if (f)
    {
        byte era = *(byte *)MAP_ERA;
        if (era == 0)
            new_grp = grp_port_forest;
        else if (era == 1)
            new_grp = grp_port_winter;
        else if (era == 2)
            new_grp = grp_port_wast;
        else if (era == 3)
            new_grp = grp_port_swamp;
    }

    if (new_grp)
        return ((void (*)(void *, byte, int, int))g_proc_004453A7)(new_grp, frame, b, c);
    else
        return ((void (*)(void *, byte, int, int))g_proc_004453A7)(grp, frame, b, c); // original
}

PROC g_proc_004452B0; // draw buttons
void grp_draw_portrait_buttons(void *grp, byte frame, int b, int c)
{
    bool f = true;
    void *new_grp = NULL;
    //-------------------------------------------------
    int *u = NULL;
    // int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int *)*(int *)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)
            break;
    }
    if (u != NULL)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if (id == U_HLUMBER)
        {
            if (frame == 1)
            {
                new_grp = grp_saw;
                f = false;
            }
            else if (frame == 2)
            {
                new_grp = grp_chop;
                frame = 0;
                f = false;
            }
        }
        else if (id == U_FARM)
        {
            if (frame == 4)
            {
                new_grp = grp_farm;
                f = false;
            }
        }
        else if (id == U_CHURCH)
        {
            if (frame == 1)
            {
                new_grp = grp_beer;
                f = false;
            }
        }
        else if (id == U_STABLES)
        {
            if (frame == 1)
            {
                new_grp = grp_instr;
                f = false;
            }
            else if (frame == 2)
            {
                new_grp = grp_alebard;
                frame = 1;
                f = false;
            }
            else if (frame == 3)
            {
                new_grp = grp_leg;
                frame = 1;
                f = false;
            }
        }
        else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if (frame == 1)
            {
                new_grp = grp_regen;
                f = false;
            }
        }
        else if (id == U_DANATH)
        {
            if (frame == 4)
            {
                new_grp = grp_shield;
                frame = 0;
                f = false;
            }
            else if (frame == 3)
            {
                new_grp = grp_bash;
                frame = 0;
                f = false;
            }
        }
    }
    //-------------------------------------------------
    // new_grp = grp_port;

    if (f)
    {
        byte era = *(byte *)MAP_ERA;
        if (era == 0)
            new_grp = grp_port_forest;
        else if (era == 1)
            new_grp = grp_port_winter;
        else if (era == 2)
            new_grp = grp_port_wast;
        else if (era == 3)
            new_grp = grp_port_swamp;
    }

    if (new_grp)
        return ((void (*)(void *, byte, int, int))g_proc_004452B0)(new_grp, frame, b, c);
    else
        return ((void (*)(void *, byte, int, int))g_proc_004452B0)(grp, frame, b, c); // original
}

PROC g_proc_0044A65C;
int status_get_tbl(void *tbl, WORD str_id)
{
    int *u = (int *)*(int *)UNIT_STATUS_TBL;
    void *new_tbl = NULL;
    //-------------------------------------------------
    if (u != NULL)
    {
        if (str_id < 268)
        {
            byte id = *((byte *)((uintptr_t)u + S_ID));
            byte ow = *((byte*)((uintptr_t)u + S_OWNER));
            if (id == U_KARGATH)
            {
                if (ow == P_NEUTRAL)
                {
                    new_tbl = tbl_names_secret3;
                    str_id = 2;
                }
                else
                {
                    new_tbl = tbl_name1;
                    str_id = 1;
                }
            }
            else if (id == U_TERON)
            {
                new_tbl = tbl_name8;
                str_id = 1;
            }
            else if (id == U_CHOGAL)
            {
                if (ow == P_NEUTRAL)
                {
                    new_tbl = tbl_names_secret3;
                    str_id = 3;
                }
                else
                {
                    new_tbl = tbl_name10;
                    str_id = 1;
                }
            }
            else if (id == U_HLUMBER)
            {
                new_tbl = tbl_name11;
                str_id = 1;
            }
            else if (id == U_DANATH)
            {
                if (hero_name_loaded)
                {
                    sprintf(hero_full_name, "%s %s", text_from_tbl(tbl_name2, 0), hero_name);
                    return (int)hero_full_name;
                }
                else
                {
                    new_tbl = tbl_name2;
                    str_id = 1;
                }
            }
            else if (id == U_ARCHER)
            {
                new_tbl = tbl_name3;
                str_id = 1;
            }
            else if (id == U_HDESTROYER)
            {
                new_tbl = tbl_name4;
                str_id = 1;
            }
            else if (id == U_PEASANT)
            {
                new_tbl = tbl_name12;
                str_id = 1;
            }
            else if (id == U_FOOTMAN)
            {
                new_tbl = tbl_name13;
                str_id = 1;
            }
            else if (id == U_KNIGHT)
            {
                new_tbl = tbl_name14;
                str_id = 1;
            }
            else if (id == U_ATTACK_PEASANT)
            {
                new_tbl = tbl_name15;
                str_id = 1;
            }
            else if (id == U_BALLISTA)
            {
                new_tbl = tbl_name16;
                str_id = 1;
            }
            else if (id == U_DWARWES)
            {
                new_tbl = tbl_name17;
                str_id = 1;
            }
            else if (id == U_EYE)
            {
                new_tbl = tbl_name18;
                str_id = 1;
            }
            else if (id == U_GRUNT)
            {
                new_tbl = tbl_name19;
                str_id = 1;
            }
            else if (id == U_PEON)
            {
                new_tbl = tbl_name20;
                str_id = 1;
            }
            else if (id == U_GROM)
            {
            if (ow == P_NEUTRAL)
            {
                new_tbl = tbl_names_secret3;
                str_id = 1;
            }
            else
            {
                new_tbl = tbl_name21;
                str_id = 1;
            }
            }
            else if (id == U_ZULJIN)
            {
                new_tbl = tbl_name22;
                str_id = 1;
            }
            else if (id == U_DENTARG)
            {
                if (ow == P_NEUTRAL)
                {
                    new_tbl = tbl_names_secret3;
                    str_id = 4;
                }
                else
                {
                    new_tbl = tbl_name23;
                    str_id = 1;
                }
            }
            else if (id == U_FLYER)
            {
                new_tbl = tbl_name24;
                str_id = 1;
            }
            else if (id == U_TOWN_HALL)
            {
                new_tbl = tbl_name25;
                str_id = 1;
            }
            else if (id == U_KEEP)
            {
                new_tbl = tbl_name26;
                str_id = 1;
            }
            else if (id == U_HBARRACK)
            {
                new_tbl = tbl_name27;
                str_id = 1;
            }
            else if (id == U_STABLES)
            {
                new_tbl = tbl_name28;
                str_id = 1;
            }
            else if (id == U_CHURCH)
            {
                new_tbl = tbl_name29;
                str_id = 1;
            }
            else if (id == U_INVENTOR)
            {
                new_tbl = tbl_name30;
                str_id = 1;
            }
            else if (id == U_HTOWER)
            {
                new_tbl = tbl_name31;
                str_id = 1;
            }
            else if (id == U_HARROWTOWER)
            {
                new_tbl = tbl_name32;
                str_id = 1;
            }
            else if (id == U_HCANONTOWER)
            {
                new_tbl = tbl_name33;
                str_id = 1;
            }
            else if (id == U_OIL)
            {
                new_tbl = tbl_name34;
                str_id = 1;
            }
            else if (id == U_SUBMARINE)
            {
                new_tbl = tbl_name35;
                str_id = 1;
            }
        }
        if (str_id == 0x1BE)
        {
            new_tbl = tbl_error1;
            str_id = 1;
        }
    }
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0044A65C)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0044A65C)(tbl, str_id); // original
}

int *hover_unit;

PROC g_proc_0044AC83;
void unit_hover_get_id(int a, int *b)
{
    if (b != NULL)
    {
        byte id = *((byte *)((uintptr_t)b + 0x20));
        hover_unit = (int *)*(int *)(LOCAL_UNIT_SELECTED_PANEL + 4 * id);
    }
    else
        hover_unit = NULL;
    ((void (*)(int, int *))g_proc_0044AC83)(a, b); // original
}

PROC g_proc_0044AE27;
int unit_hover_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    int *u = hover_unit;
    if (u != NULL)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        byte ow = *((byte*)((uintptr_t)u + S_OWNER));
        if (id == U_KARGATH)
        {
            if (ow == P_NEUTRAL)
            {
                new_tbl = tbl_names_secret3;
                str_id = 2;
            }
            else
            {
                new_tbl = tbl_name1;
                str_id = 1;
            }
        }
        else if (id == U_TERON)
        {
            new_tbl = tbl_name8;
            str_id = 1;
        }
        else if (id == U_CHOGAL)
        {
            if (ow == P_NEUTRAL)
            {
                new_tbl = tbl_names_secret3;
                str_id = 3;
            }
            else
            {
                new_tbl = tbl_name10;
                str_id = 1;
            }
        }
        else if (id == U_HLUMBER)
        {
            new_tbl = tbl_name11;
            str_id = 1;
        }
        else if (id == U_DANATH)
        {
            if (hero_name_loaded)
            {
                return (int)hero_name;
            }
            else
            {
                new_tbl = tbl_name2;
                str_id = 1;
            }
        }
        else if (id == U_ARCHER)
        {
            new_tbl = tbl_name3;
            str_id = 1;
        }
        else if (id == U_HDESTROYER)
        {
            new_tbl = tbl_name4;
            str_id = 1;
        }
        else if (id == U_PEASANT)
        {
            new_tbl = tbl_name12;
            str_id = 1;
        }
        else if (id == U_FOOTMAN)
        {
            new_tbl = tbl_name13;
            str_id = 1;
        }
        else if (id == U_KNIGHT)
        {
            new_tbl = tbl_name14;
            str_id = 1;
        }
        else if (id == U_ATTACK_PEASANT)
        {
            new_tbl = tbl_name15;
            str_id = 1;
        }
        else if (id == U_BALLISTA)
        {
            new_tbl = tbl_name16;
            str_id = 1;
        }
        else if (id == U_DWARWES)
        {
            new_tbl = tbl_name17;
            str_id = 1;
        }
        else if (id == U_EYE)
        {
            new_tbl = tbl_name18;
            str_id = 1;
        }
        else if (id == U_GRUNT)
        {
            new_tbl = tbl_name19;
            str_id = 1;
        }
        else if (id == U_PEON)
        {
            new_tbl = tbl_name20;
            str_id = 1;
        }
        else if (id == U_GROM)
        {
            if (ow == P_NEUTRAL)
            {
                new_tbl = tbl_names_secret3;
                str_id = 1;
            }
            else
            {
                new_tbl = tbl_name21;
                str_id = 1;
            }
        }
        else if (id == U_ZULJIN)
        {
            new_tbl = tbl_name22;
            str_id = 1;
        }
        else if (id == U_DENTARG)
        {
            if (ow == P_NEUTRAL)
            {
                new_tbl = tbl_names_secret3;
                str_id = 4;
            }
            else
            {
                new_tbl = tbl_name23;
                str_id = 1;
            }
        }
        else if (id == U_FLYER)
        {
            new_tbl = tbl_name24;
            str_id = 1;
        }
        else if (id == U_TOWN_HALL)
        {
            new_tbl = tbl_name25;
            str_id = 1;
        }
        else if (id == U_KEEP)
        {
            new_tbl = tbl_name26;
            str_id = 1;
        }
        else if (id == U_HBARRACK)
        {
            new_tbl = tbl_name27;
            str_id = 1;
        }
        else if (id == U_STABLES)
        {
            new_tbl = tbl_name28;
            str_id = 1;
        }
        else if (id == U_CHURCH)
        {
            new_tbl = tbl_name29;
            str_id = 1;
        }
        else if (id == U_INVENTOR)
        {
            new_tbl = tbl_name30;
            str_id = 1;
        }
        else if (id == U_HTOWER)
        {
            new_tbl = tbl_name31;
            str_id = 1;
        }
        else if (id == U_HARROWTOWER)
        {
            new_tbl = tbl_name32;
            str_id = 1;
        }
        else if (id == U_HCANONTOWER)
        {
            new_tbl = tbl_name33;
            str_id = 1;
        }
        else if (id == U_OIL)
        {
            new_tbl = tbl_name34;
            str_id = 1;
        }
        else if (id == U_SUBMARINE)
        {
            new_tbl = tbl_name35;
            str_id = 1;
        }
    }
    //-------------------------------------------------
    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0044AE27)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0044AE27)(tbl, str_id); // original
}

PROC g_proc_0044AE56;
void button_description_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    int *u = NULL;
    // int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int *)*(int *)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)
            break;
    }
    if (u != NULL)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if (str_id == 1)
                new_tbl = tbl_skill2;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill3;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_skill1;
                str_id = 1;
            }
        }
        else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if (str_id == 1)
                new_tbl = tbl_train2;
            else if (str_id == 2)
            {
                new_tbl = tbl_research2;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_train1;
                str_id = 1;
            }
            else if (str_id == 4)
            {
                new_tbl = tbl_train3;
                str_id = 1;
            }
        }
        else if (id == U_HLUMBER)
        {
            if (str_id == 1)
                new_tbl = tbl_research8;
            else if (str_id == 2)
            {
                new_tbl = tbl_research3;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research11;
                str_id = 1;
            }
        }
        else if (id == U_FARM)
        {
            if (str_id == 1)
                new_tbl = tbl_research1;
        }
        else if (id == U_CHURCH)
        {
            if (str_id == 1)
                new_tbl = tbl_skill4;
        }
        else if (id == U_STABLES)
        {
            if (str_id == 1)
                new_tbl = tbl_research4;
            else if (str_id == 2)
            {
                new_tbl = tbl_research5;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research6;
                str_id = 1;
            }
        }
        else if (id == U_HSMITH)
        {
            if (str_id == 1)
                new_tbl = tbl_research7;
            else if (str_id == 2)
            {
                new_tbl = tbl_research7;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
            else if (str_id == 4)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
            else if (str_id == 5)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
            else if (str_id == 6)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
        }
        else if (id == U_HBARRACK)
        {
            if (str_id == 1)
                new_tbl = tbl_train4;
            else if (str_id == 2)
            {
                new_tbl = tbl_train5;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_train6;
                str_id = 1;
            }
        }
        else if (id == U_PEASANT)
        {
            if (str_id == 50)
            {
                new_tbl = tbl_build1;
                str_id = 1;
            }
            else if (str_id == 51)
            {
                new_tbl = tbl_build2;
                str_id = 1;
            }
            else if (str_id == 52)
            {
                new_tbl = tbl_build3;
                str_id = 1;
            }
            else if (str_id == 53)
            {
                new_tbl = tbl_build4;
                str_id = 1;
            }
            else if (str_id == 54)
            {
                new_tbl = tbl_build5;
                str_id = 1;
            }
        }
    }
    //-------------------------------------------------
    // new_tbl = tbl_names;

    if (new_tbl)
        return ((void (*)(void *, int))g_proc_0044AE56)(new_tbl, str_id);
    else
        return ((void (*)(void *, int))g_proc_0044AE56)(tbl, str_id); // original
}

PROC g_proc_0044B23D;
void button_hotkey_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    int *u = NULL;
    // int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int *)*(int *)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)
            break;
    }
    if (u != NULL)
    {
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if (str_id == 1)
                new_tbl = tbl_skill2;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill3;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_skill1;
                str_id = 1;
            }
        }
        else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if (str_id == 1)
                new_tbl = tbl_train2;
            else if (str_id == 2)
            {
                new_tbl = tbl_research2;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_train1;
                str_id = 1;
            }
            else if (str_id == 4)
            {
                new_tbl = tbl_train3;
                str_id = 1;
            }
        }
        else if (id == U_HLUMBER)
        {
            if (str_id == 1)
                new_tbl = tbl_research8;
            else if (str_id == 2)
            {
                new_tbl = tbl_research3;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research11;
                str_id = 1;
            }
        }
        else if (id == U_FARM)
        {
            if (str_id == 1)
                new_tbl = tbl_research1;
        }
        else if (id == U_CHURCH)
        {
            if (str_id == 1)
                new_tbl = tbl_skill4;
        }
        else if (id == U_STABLES)
        {
            if (str_id == 1)
                new_tbl = tbl_research4;
            else if (str_id == 2)
            {
                new_tbl = tbl_research5;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research6;
                str_id = 1;
            }
        }
        else if (id == U_HSMITH)
        {
            if (str_id == 1)
                new_tbl = tbl_research7;
            else if (str_id == 2)
            {
                new_tbl = tbl_research7;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
            else if (str_id == 4)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
            else if (str_id == 5)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
            else if (str_id == 6)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
        }
        else if (id == U_HBARRACK)
        {
            if (str_id == 1)
                new_tbl = tbl_train4;
            else if (str_id == 2)
            {
                new_tbl = tbl_train5;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_train6;
                str_id = 1;
            }
        }
        else if (id == U_PEASANT)
        {
            if (str_id == 50)
            {
                new_tbl = tbl_build1;
                str_id = 1;
            }
            else if (str_id == 51)
            {
                new_tbl = tbl_build2;
                str_id = 1;
            }
            else if (str_id == 52)
            {
                new_tbl = tbl_build3;
                str_id = 1;
            }
            else if (str_id == 53)
            {
                new_tbl = tbl_build4;
                str_id = 1;
            }
            else if (str_id == 54)
            {
                new_tbl = tbl_build5;
                str_id = 1;
            }
        }
    }
    //-------------------------------------------------
    // new_tbl = tbl_names;

    if (new_tbl)
        return ((void (*)(void *, int))g_proc_0044B23D)(new_tbl, str_id);
    else
        return ((void (*)(void *, int))g_proc_0044B23D)(tbl, str_id); // original
}

char tbl_kill[] = "Kill all";

PROC g_proc_004354C8;
int objct_get_tbl_custom(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_kill;
    //-------------------------------------------------
    // new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004354C8)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004354C8)(tbl, str_id); // original
}

PROC g_proc_004354FA;
int objct_get_tbl_campanign(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
        new_tbl = tbl_task1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        new_tbl = tbl_task2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        new_tbl = tbl_task3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        new_tbl = tbl_task4;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        new_tbl = tbl_task5;
    else if (lvl == LVL_ORC1)
        new_tbl = tbl_task_secret;
    str_id = 1;
    //-------------------------------------------------
    // new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004354FA)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004354FA)(tbl, str_id); // original
}

PROC g_proc_004300A5;
int objct_get_tbl_briefing_task(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
        new_tbl = tbl_task1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        new_tbl = tbl_task2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        new_tbl = tbl_task3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        new_tbl = tbl_task4;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        new_tbl = tbl_task5;
    else if (lvl == LVL_ORC1)
        new_tbl = tbl_task_secret;
    str_id = 1;
    //-------------------------------------------------
    // new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004300A5)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004300A5)(tbl, str_id); // original
}

PROC g_proc_004300CA;
int objct_get_tbl_briefing_title(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
        new_tbl = tbl_title1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        new_tbl = tbl_title2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        new_tbl = tbl_title3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        new_tbl = tbl_title4;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        new_tbl = tbl_title5;
    else if (lvl == LVL_ORC1)
        new_tbl = tbl_title_secret;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004300CA)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004300CA)(tbl, str_id); // original
}

PROC g_proc_004301CA;
int objct_get_tbl_briefing_text(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
        new_tbl = tbl_brif1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        new_tbl = tbl_brif2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        new_tbl = tbl_brif3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        new_tbl = tbl_brif4;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        new_tbl = tbl_brif5;
    else if (lvl == LVL_ORC1)
        new_tbl = tbl_brif_secret;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004301CA)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004301CA)(tbl, str_id); // original
}

char story1[] = "RedMistPrequel\\storyteller\\1.wav";
char story2[] = "RedMistPrequel\\storyteller\\2.wav";
char story3[] = "RedMistPrequel\\storyteller\\3.wav";
char story4[] = "RedMistPrequel\\storyteller\\4.wav";
char story5[] = "RedMistPrequel\\storyteller\\5.wav";
char story_secret[] = "RedMistPrequel\\storyteller\\secret.wav";
char story_end[] = "RedMistPrequel\\storyteller\\end.wav";
char story_end2[] = "RedMistPrequel\\storyteller\\end2.wav";

void set_speech(char *speech, char *adr)
{
    patch_setdword((DWORD *)(speech + 4), (DWORD)adr);
    patch_setdword((DWORD *)(speech + 12), 0);
}

DWORD remember_music = 101;
DWORD remember_sound = 101;

PROC g_proc_00430113;
int objct_get_briefing_speech(char *speech)
{
    remember_music = *(DWORD *)VOLUME_MUSIC;
    remember_sound = *(DWORD *)VOLUME_SOUND;
    if (remember_music != 0)
        *(DWORD *)VOLUME_MUSIC = 20;
    *(DWORD *)VOLUME_SOUND = 100;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM); // set volume

    DWORD remember1 = *(DWORD *)(speech + 4);
    DWORD remember2 = *(DWORD *)(speech + 12);
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
        set_speech(speech, story1);
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        set_speech(speech, story2);
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        set_speech(speech, story3);
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        set_speech(speech, story4);
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        set_speech(speech, story5);
    else if (lvl == LVL_ORC1)
        set_speech(speech, story_secret);
    //-------------------------------------------------

    int original = ((int (*)(char *))g_proc_00430113)(speech); // original
    patch_setdword((DWORD *)(speech + 4), remember1);
    patch_setdword((DWORD *)(speech + 12), remember2);
    return original;
}

bool finale_dlg = false;

PROC g_proc_0041F0F5;
int finale_get_tbl(void *tbl, WORD str_id)
{
    finale_dlg = false;
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl == (LVL_XHUMAN12 + 2))
    {
        new_tbl = tbl_end2;
        str_id = 1;
    }
    else
    {
        new_tbl = tbl_end;
        str_id = 1;
    }
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0041F0F5)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0041F0F5)(tbl, str_id); // original
}

PROC g_proc_0041F1E8;
int finale_credits_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
        new_tbl = tbl_credits;
        str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0041F1E8)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0041F1E8)(tbl, str_id); // original
}

PROC g_proc_0041F027;
int finale_get_speech(char *speech)
{
    remember_music = *(DWORD *)VOLUME_MUSIC;
    remember_sound = *(DWORD *)VOLUME_SOUND;
    if (remember_music != 0)*(DWORD *)VOLUME_MUSIC = 22;
    *(DWORD *)VOLUME_SOUND = 100;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM); // set volume

    DWORD remember1 = *(DWORD *)(speech + 4);
    DWORD remember2 = *(DWORD *)(speech + 12);

    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl == (LVL_XHUMAN12 + 2))
        set_speech(speech, story_end2);
    else
        set_speech(speech, story_end);
    //-------------------------------------------------
    int original = ((int (*)(char *))g_proc_0041F027)(speech); // original
    patch_setdword((DWORD *)(speech + 4), remember1);
    patch_setdword((DWORD *)(speech + 12), remember2);
    return original;
}

int cred_num = 0;

PROC g_proc_00417E33;
int credits_small_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    new_tbl = tbl_credits;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_00417E33)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_00417E33)(tbl, str_id); // original
}

PROC g_proc_00417E4A;
int credits_big_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_credits_title;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_00417E4A)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_00417E4A)(tbl, str_id); // original
}

char tbl_empty[] = " ";

PROC g_proc_0042968A;
int act_get_tbl_small(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    return (int)tbl_empty;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0042968A)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0042968A)(tbl, str_id); // original
}

PROC g_proc_004296A9;
int act_get_tbl_big(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    byte lvl = *(byte *)LEVEL_OBJ;
    //-------------------------------------------------
    return (int)tbl_empty;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_004296A9)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_004296A9)(tbl, str_id); // original
}

PROC g_proc_0041C51C;
int netstat_get_tbl_nation(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    // 46 blue hum
    // 47 blue orc
    // 48 white hum
    // 49 white orc
    // 50 red hum
    // 51 red orc
    // 52 green hum
    // 53 green orc
    // 54 black hum
    // 55 black orc
    // 56 violet hum
    // 57 violet orc
    // 58 orange hum
    // 59 orange orc
    // 60 yellow hum
    // 61 yellow orc
    // 92 blue xorc
    // 93 white xorc
    // 94 red xorc
    // 95 green xorc
    // 96 black xorc
    // 97 violet xorc
    // 98 orange xorc
    // 99 yellow xorc
    //-------------------------------------------------
    if (str_id == 46)
    {
        new_tbl = tbl_nations;
        str_id = 1;
    }
    else if (str_id == 54)
    {
        new_tbl = tbl_nations;
        str_id = 3;
    }
    else if ((str_id == 52) || (str_id == 60) || (str_id == 58) || (str_id == 56) || (str_id == 48))
    {
        new_tbl = tbl_nations;
        str_id = 4;
    }
    else if (((str_id >= 46) && (str_id <= 61)) || ((str_id >= 92) && (str_id <= 99)))
    {
        new_tbl = tbl_nations;
        str_id = 2;
    }
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_0041C51C)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_0041C51C)(tbl, str_id); // original
}

PROC g_proc_00431229;
int rank_get_tbl(void *tbl, WORD str_id)
{
    void *new_tbl = NULL;
    //-------------------------------------------------

    //-------------------------------------------------
    // new_tbl = tbl_rank;

    if (new_tbl)
        return ((int (*)(void *, int))g_proc_00431229)(new_tbl, str_id);
    else
        return ((int (*)(void *, int))g_proc_00431229)(tbl, str_id); // original
}

char instal2[] = "\\RedMistPrequel\\fonts.mpq";
DWORD loaded_instal2 = 0;

char my_font6[] = "RedMistFonts\\font6.fnt";
char my_font10x[] = "RedMistFonts\\font10x.fnt";
char my_font12x[] = "RedMistFonts\\font12x.fnt";
char my_font32[] = "RedMistFonts\\font32.fnt";
char my_font50[] = "RedMistFonts\\font50.fnt";

void reload_install_exe2()
{
    if (loaded_instal2 == 0)
    {
        *(DWORD *)INSTALL_EXE_POINTER = 0; // remove existing install
        char buf[] = "\x0\x0\x0\x0";
        patch_setdword((DWORD *)buf, (DWORD)instal2);
        PATCH_SET((char *)INSTALL_EXE_NAME1, buf);
        PATCH_SET((char *)INSTALL_EXE_NAME2, buf); // change names
        PATCH_SET((char *)INSTALL_EXE_NAME3, buf);
        ((int (*)(int, int))F_RELOAD_INSTALL_EXE)(1, 0); // load install.exe
        loaded_instal2 = *(DWORD *)INSTALL_EXE_POINTER;
    }
    else
    {
        *(DWORD *)INSTALL_EXE_POINTER = loaded_instal2;
    }
}

PROC g_proc_00424A9C; // 32
PROC g_proc_00424AB2; // 50
PROC g_proc_004288B2; // 12
PROC g_proc_00428896; // 10
PROC g_proc_0042887D; // 6
void *storm_font_load(char *name, char *a1, int a2)
{
    DWORD orig_instal = *(DWORD *)INSTALL_EXE_POINTER; // remember existing install
    reload_install_exe2();
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font6.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font6;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font10x.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font10x;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font12x.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font12x;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font32.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font32;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font50.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font50;
    }
    void *original = ((void *(*)(char *, char *, int))g_proc_00424A9C)(name, a1, a2); // original
    *(DWORD *)INSTALL_EXE_POINTER = orig_instal;                                      // restore existing install
    return original;
}

void small_fonts_reload()
{
    DWORD orig_instal = *(DWORD *)INSTALL_EXE_POINTER; // remember existing install
    reload_install_exe2();
    *(DWORD *)FONT_POINTER_6 = ((DWORD(*)(char *, char *, int))F_FONT_RELOAD)(my_font6, (char *)FONT_RELOAD_PARAM, 0x3B9);    // original storm font load
    *(DWORD *)FONT_POINTER_10 = ((DWORD(*)(char *, char *, int))F_FONT_RELOAD)(my_font10x, (char *)FONT_RELOAD_PARAM, 0x3BA); // original storm font load
    *(DWORD *)FONT_POINTER_12 = ((DWORD(*)(char *, char *, int))F_FONT_RELOAD)(my_font12x, (char *)FONT_RELOAD_PARAM, 0x3BB); // original storm font load
    *(DWORD *)INSTALL_EXE_POINTER = orig_instal;                                                                              // restore existing install
}

void pal_load(byte *palette_adr, void *pal)
{
    if (palette_adr != NULL)
    {
        if (pal != NULL)
        {
            DWORD i = 0;
            while (i < (256 * 4))
            {
                *(byte *)(palette_adr + i) = *(byte *)((DWORD)pal + i);
                i++;
            }
        }
    }
}

char new_menu[] = "RedMistPrequel\\titlemenu_bne.pcx";
char new_splash[] = "RedMistPrequel\\title_rel_bne.pcx";

PROC g_proc_004372EE;
void pcx_load_menu(char* name, void* pcx_info, byte* palette_adr)
{
    small_fonts_reload();
    char* new_name = name;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\titlemenu_bne.pcx", -1) == CSTR_EQUAL)
    {
        new_name = new_menu;
    }
    else if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\title_rel_bne.pcx", -1) == CSTR_EQUAL)
    {
        new_name = new_splash;
    }
    ((void (*)(char*, void*, byte*))g_proc_004372EE)(new_name, pcx_info, palette_adr);//original
}

PROC g_proc_00430058;
void pcx_load_briefing(char *name, void *pcx_info, byte *palette_adr)
{
    ((void (*)(char *, void *, byte *))g_proc_00430058)(name, pcx_info, palette_adr); // original
    byte lvl = *(byte *)LEVEL_OBJ;
    void *new_pcx_pixels = NULL;
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
    {
        new_pcx_pixels = pcx_brif1;
        pal_load(palette_adr, pcx_brif1_pal);
    }
    if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
    {
        new_pcx_pixels = pcx_brif2;
        pal_load(palette_adr, pcx_brif2_pal);
    }
    if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
    {
        new_pcx_pixels = pcx_brif3;
        pal_load(palette_adr, pcx_brif3_pal);
    }
    if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
    {
        new_pcx_pixels = pcx_brif4;
        pal_load(palette_adr, pcx_brif4_pal);
    }
    if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
    {
        new_pcx_pixels = pcx_brif5;
        pal_load(palette_adr, pcx_brif5_pal);
    }
    if (lvl == LVL_ORC1)
    {
        new_pcx_pixels = pcx_brif_secret;
        pal_load(palette_adr, pcx_brif_secret_pal);
    }

    if (new_pcx_pixels)
        patch_setdword((DWORD *)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00429625; // load palette
PROC g_proc_00429654; // load image
void pcx_load_act(char *name, void *pcx_info, byte *palette_adr)
{
    ((void (*)(char *, void *, byte *))g_proc_00429625)(name, pcx_info, palette_adr); // original
    byte lvl = *(byte *)LEVEL_OBJ;
    void *new_pcx_pixels = NULL;

    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
    {
        new_pcx_pixels = pcx_act1;
        pal_load(palette_adr, pcx_act1_pal);
    }
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
    {
        new_pcx_pixels = pcx_act2;
        pal_load(palette_adr, pcx_act2_pal);
    }
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
    {
        new_pcx_pixels = pcx_act3;
        pal_load(palette_adr, pcx_act3_pal);
    }
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
    {
        new_pcx_pixels = pcx_act4;
        pal_load(palette_adr, pcx_act4_pal);
    }
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
    {
        new_pcx_pixels = pcx_act5;
        pal_load(palette_adr, pcx_act5_pal);
    }
    else if (lvl == LVL_ORC1)
    {
        new_pcx_pixels = pcx_act_secret;
        pal_load(palette_adr, pcx_act_secret_pal);
    }
    else
    {
        new_pcx_pixels = pcx_act1;
        pal_load(palette_adr, pcx_act1_pal);
    }

    if (new_pcx_pixels)
        patch_setdword((DWORD *)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0041F004;
void pcx_load_final(char *name, void *pcx_info, byte *palette_adr)
{
    finale_dlg = true;
    ((void (*)(char *, void *, byte *))g_proc_0041F004)(name, pcx_info, palette_adr); // original
    void *new_pcx_pixels = NULL;
    if (*(byte*)LEVEL_OBJ == LVL_XHUMAN12 + 2)
    {
        new_pcx_pixels = pcx_end2;
        pal_load(palette_adr, pcx_end2_pal);
    }
    else
    {
        new_pcx_pixels = pcx_end;
        pal_load(palette_adr, pcx_end_pal);
    }
    if (new_pcx_pixels)
        patch_setdword((DWORD *)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00417DDB;
void pcx_load_credits(char *name, void *pcx_info, byte *palette_adr)
{
    ((void (*)(char *, void *, byte *))g_proc_00417DDB)(name, pcx_info, palette_adr); // original
    void *new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\credits.pcx", -1) == CSTR_EQUAL)
    {
        if (cred_num == 0)
        {
            cred_num = 1;
            new_pcx_pixels = pcx_credits;
            pal_load(palette_adr, pcx_credits_pal);
        }
        if (cred_num == 2)
        {
            cred_num = 3;
            new_pcx_pixels = pcx_credits;
            pal_load(palette_adr, pcx_credits_pal);
        }
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\2xcredit.pcx", -1) == CSTR_EQUAL)
    {
        cred_num = 2;
        new_pcx_pixels = pcx_credits;
        pal_load(palette_adr, pcx_credits_pal);
    }
    if (new_pcx_pixels)
        patch_setdword((DWORD *)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0043169E;
void pcx_load_statistic(char *name, void *pcx_info, byte *palette_adr)
{
    if (*(byte*)LEVEL_OBJ == LVL_XHUMAN12)*(byte*)PLAYER_RACE = 0;

    ((void (*)(char *, void *, byte *))g_proc_0043169E)(name, pcx_info, palette_adr); // original
    void *new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hwinglu.pcx", -1) == CSTR_EQUAL)
    {
        new_pcx_pixels = pcx_win;
        pal_load(palette_adr, pcx_win_pal);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hlosglu.pcx", -1) == CSTR_EQUAL)
    {
        new_pcx_pixels = pcx_loss;
        pal_load(palette_adr, pcx_loss_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD *)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

const char RECORD_PATH[] = ".\\RedMistPrequel\\record.save";

PROC g_proc_00462D4D;
void *storm_file_load(char *name, int a1, int a2, int a3, int a4, int a5, int a6)
{
    void *original = ((void *(*)(void *, int, int, int, int, int, int))g_proc_00462D4D)(name, a1, a2, a3, a4, a5, a6); // original
    void *new_file = NULL;
    //-------------------------------------------------
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\mainmenu.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_menu_copy, bin_menu, bin_menu_size);
        new_file = bin_menu_copy;
        patch_setdword((DWORD *)a2, bin_menu_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\snglplay.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_sngl_copy, bin_sngl, bin_sngl_size);
        new_file = bin_sngl_copy;
        patch_setdword((DWORD *)a2, bin_sngl_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\newcmpgn.bin", -1) == CSTR_EQUAL)
    {
        bool secret = false;
        FILE* fp;
        if ((fp = fopen(RECORD_PATH, "rb")) != NULL)//file opened
        {
            secret = true;
            fclose(fp);
        }
        if (secret)
        {
            memcpy(bin_newcmp2_copy, bin_newcmp2, bin_newcmp2_size);
            new_file = bin_newcmp2_copy;
            patch_setdword((DWORD*)a2, bin_newcmp2_size);
        }
        else
        {
            memcpy(bin_newcmp_copy, bin_newcmp, bin_newcmp_size);
            new_file = bin_newcmp_copy;
            patch_setdword((DWORD*)a2, bin_newcmp_size);
        }
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\hdisptch.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_hdisptch_copy, bin_hdisptch, bin_hdisptch_size);
        new_file = bin_hdisptch_copy;
        patch_setdword((DWORD*)a2, bin_hdisptch_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\odisptch.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_hdisptch_copy, bin_hdisptch, bin_hdisptch_size);
        new_file = bin_hdisptch_copy;
        patch_setdword((DWORD*)a2, bin_hdisptch_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\quit.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_quit_copy, bin_quit, bin_quit_size);
        new_file = bin_quit_copy;
        patch_setdword((DWORD *)a2, bin_quit_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\creditsBNE.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_credits_copy, bin_credits, bin_credits_size);
        new_file = bin_credits_copy;
        patch_setdword((DWORD*)a2, bin_credits_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\ai.bin", -1) == CSTR_EQUAL)
    {
        new_file = bin_AI;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\script.bin", -1) == CSTR_EQUAL)
    {
        new_file = bin_script;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\unitdata.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_unitdata;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\unitdato.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_unitdato;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\upgrades.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_upgrades;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\bgs\\iceland\\iceland.cv4", -1) == CSTR_EQUAL)
    {
        new_file = tile_iceland_cv4;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\bgs\\swamp\\swamp.cv4", -1) == CSTR_EQUAL)
    {
        new_file = tile_desert_cv4;
    }
    //-------------------------------------------------
    if (new_file)
        return new_file;
    else
        return original;
}

void set_rgb(DWORD *adr, byte r, byte g, byte b)
{
    patch_setdword(adr, r + (g << 8) + (b << 16));
}

void set_rgb4(DWORD *adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (4 * r) + ((4 * g) << 8) + ((4 * b) << 16));
}

void set_rgb8(DWORD *adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (8 * r) + ((8 * g) << 8) + ((8 * b) << 16));
}

void set_palette(void *pal)
{
    for (int i = 0; i < 256; i++)
    {
        byte r = *(byte *)((int)pal + 3 * i);
        byte g = *(byte *)((int)pal + 3 * i + 1);
        byte b = *(byte *)((int)pal + 3 * i + 2);
        set_rgb4((DWORD *)(SCREEN_INITIAL_PALETTE + i * 4), r, g, b);
    }
}

PROC g_proc_0041F9FD;
int tilesets_load(int a)
{
    int original = ((int (*)(int))g_proc_0041F9FD)(a);
    byte lvl = *(byte *)LEVEL_OBJ;
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1) || (lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2) || (lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
    {
        patch_setdword((DWORD*)TILESET_POINTER_VX4, (DWORD)tile_desert_vx4);
        patch_setdword((DWORD*)TILESET_POINTER_VR4, (DWORD)tile_desert_vr4);
        //cv4 in storm_file_load
        set_palette(tile_desert_palette);
    }
    if (lvl == LVL_ORC1)
    {
        patch_setdword((DWORD*)TILESET_POINTER_VX4, (DWORD)tile_iceland_vx4);
        patch_setdword((DWORD*)TILESET_POINTER_VR4, (DWORD)tile_iceland_vr4);
        //cv4 in storm_file_load
        set_palette(tile_iceland_palette);
        set_rgb((DWORD*)(SCREEN_INITIAL_PALETTE + RAINDROPS_COLOR * 4), 255, 255, 255);
    }
    return original;
}

PROC g_proc_0041F97D;
int map_file_load(int a, int b, void **map, DWORD *size)
{
    byte lvl = *(byte *)LEVEL_OBJ;
    byte d = *(byte *)GB_DEMO; // demo
    bool f = false;
    if (d == 1)
    {
        //*map = pud_map_atr;
        //*size = pud_map_atr_size;
        // f = true;
    }
    else
    {
        if (lvl == LVL_HUMAN1)
        {
            *map = pud_map1;
            *size = pud_map1_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN2)
        {
            *map = pud_map2;
            *size = pud_map2_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN3)
        {
            *map = pud_map3;
            *size = pud_map3_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN4)
        {
            *map = pud_map4;
            *size = pud_map4_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN14)
        {
            *map = pud_map5;
            *size = pud_map5_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN1)
        {
            *map = pud_emap1;
            *size = pud_emap1_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN2)
        {
            *map = pud_emap2;
            *size = pud_emap2_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN3)
        {
            *map = pud_emap3;
            *size = pud_emap3_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN4)
        {
            *map = pud_emap4;
            *size = pud_emap4_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN12)
        {
            *map = pud_emap5;
            *size = pud_emap5_size;
            f = true;
        }
        else if (lvl == LVL_ORC1)
        {
            *map = pud_map_secret;
            *size = pud_map_secret_size;
            f = true;
        }
    }
    if (f)
        return 1;
    else
        return ((int (*)(int, int, void *, DWORD *))g_proc_0041F97D)(a, b, map, size); // original
}

void *file_load(const char name[])
{
    void *file = NULL;
    FILE *fp;
    char path[MAX_PATH] = {0};
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL) // file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp); // read
        fclose(fp);
    }
    return file;
}

void file_load_size(const char name[], void **m, DWORD *s)
{
    void *file = NULL;
    FILE *fp;
    char path[MAX_PATH] = {0};
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL) // file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        *s = size;
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp); // read
        fclose(fp);
    }
    *m = file;
}

char video_mod_shab[] = "RedMistPrequel\\videos\\%s";
char video1[] = "start.smk";
char video_novideo[] = "error.smk";

PROC g_proc_0043B16F;
void smk_play_sprintf_name(int dest, char *shab, char *name)
{
    if (!lstrcmpi(name, "intro_m.smk"))
    {
        shab = video_mod_shab;
        name = video1;
    }
    if (!lstrcmpi(name, "introx_m.smk"))
    {
        shab = video_mod_shab;
        name = video_novideo;
    }
    if (!lstrcmpi(name, "hvict_m.smk"))
    {
        shab = video_mod_shab;
        name = video_novideo;
    }
    if (!lstrcmpi(name, "hvicx_m.smk"))
    {
        shab = video_mod_shab;
        name = video_novideo;
    }
    if (!lstrcmpi(name, "orcx_m.smk"))
    {
        shab = video_mod_shab;
        name = video_novideo;
    }
    if (!lstrcmpi(name, "ovict_m.smk"))
    {
        shab = video_mod_shab;
        name = video_novideo;
    }
    ((void (*)(int, char *, char *))g_proc_0043B16F)(dest, shab, name); // original
}

PROC g_proc_0043B362;
void smk_play_sprintf_blizzard(int dest, char *shab, char *name)
{
    ((void (*)(int, char *, char *))g_proc_0043B362)(dest, video_mod_shab, video1); // original
}

char instal[] = "\\RedMistPrequel\\music.mpq";
DWORD loaded_instal = 0;
int need_instal = 0;
char music1[] = "menu.wav";
char music2[] = "end.wav";
char music3[] = "brif.wav";

void reload_install_exe()
{
    if (loaded_instal == 0)
    {
        *(DWORD *)INSTALL_EXE_POINTER = 0; // remove existing install
        char buf[] = "\x0\x0\x0\x0";
        patch_setdword((DWORD *)buf, (DWORD)instal);
        PATCH_SET((char *)INSTALL_EXE_NAME1, buf);
        PATCH_SET((char *)INSTALL_EXE_NAME2, buf); // change names
        PATCH_SET((char *)INSTALL_EXE_NAME3, buf);
        ((int (*)(int, int))F_RELOAD_INSTALL_EXE)(1, 0); // load install.exe
        loaded_instal = *(DWORD *)INSTALL_EXE_POINTER;
    }
    else
    {
        *(DWORD *)INSTALL_EXE_POINTER = loaded_instal;
    }
}

void music_play_sprintf_name(int dest, char *shab, char *name)
{
    if (!lstrcmpi(name, "owarroom.wav"))
    {
        need_instal = 2;
        name = music1;
    }
    if (!lstrcmpi(name, "hwarroom.wav"))
    {
        need_instal = 2;
        name = music3;
    }
    if (finale_dlg)
    {
        need_instal = 2;
        name = music2;
    }
    DWORD orig = *(DWORD *)F_MUSIC_SPRINTF;                  // original music sprintf call
    ((void (*)(int, char *, char *))orig)(dest, shab, name); // original
}

PROC g_proc_00440F4A;
PROC g_proc_00440F5F;
int music_play_get_install()
{
    DWORD orig_instal = *(DWORD *)INSTALL_EXE_POINTER; // remember existing install
    if (need_instal == 1)
    {
        reload_install_exe();
        need_instal--;
    }
    if (need_instal == 2)
        need_instal--;
    int original = ((int (*)())g_proc_00440F4A)(); // original
    *(DWORD *)INSTALL_EXE_POINTER = orig_instal;   // restore existing install
    return original;
}

void files_init()
{
    tbl_stats = file_load("utils\\stats.tbl");
    grp_rally_flag = file_load("utils\\rally_flag.grp");
    grp_rank = file_load("utils\\rank.grp");

    for (int i = 0; i < U_DEAD + 4; i++)
    {
        sprintf(grp_name, "default_grp\\%d.grp", i);
        grps_def[i] = file_load(grp_name);
        sprintf(grp_name, "default_grp_winter\\%d.grp", i);
        grps_winter[i] = file_load(grp_name);
        sprintf(grp_name, "default_grp_xswamp\\%d.grp", i);
        grps_xswamp[i] = file_load(grp_name);
    }

    grp_port_forest = file_load("icons\\port_forest.grp");
    grp_port_winter = file_load("icons\\port_winter.grp");
    grp_port_wast = file_load("icons\\port_wast.grp");
    grp_port_swamp = file_load("icons\\port_swamp.grp");
    grp_saw = file_load("icons\\saw.grp");
    grp_chop = file_load("icons\\chop.grp");
    grp_shield = file_load("icons\\shield.grp");
    grp_farm = file_load("icons\\farm.grp");
    grp_beer = file_load("icons\\beer.grp");
    grp_instr = file_load("icons\\instr.grp");
    grp_alebard = file_load("icons\\alebard.grp");
    grp_leg = file_load("icons\\leg.grp");
    grp_regen = file_load("icons\\regen.grp");
    grp_bash = file_load("icons\\bash.grp");

    grp_dwarf_dead = file_load("units\\death.grp");

    grp_bolt = file_load("bullets\\bolt.grp");

    tbl_credits = file_load("textes\\credits.tbl");
    tbl_brif1 = file_load("textes\\brif1.tbl");
    tbl_brif2 = file_load("textes\\brif2.tbl");
    tbl_brif3 = file_load("textes\\brif3.tbl");
    tbl_brif4 = file_load("textes\\brif4.tbl");
    tbl_brif5 = file_load("textes\\brif5.tbl");
    tbl_brif_secret = file_load("textes\\brif_secret.tbl");
    tbl_end = file_load("textes\\end.tbl");
    tbl_end2 = file_load("textes\\end2.tbl");

    tbl_task1 = file_load("textes\\task1.tbl");
    tbl_task2 = file_load("textes\\task2.tbl");
    tbl_task3 = file_load("textes\\task3.tbl");
    tbl_task4 = file_load("textes\\task4.tbl");
    tbl_task5 = file_load("textes\\task5.tbl");
    tbl_task_secret = file_load("textes\\task_secret.tbl");

    tbl_title1 = file_load("textes\\title1.tbl");
    tbl_title2 = file_load("textes\\title2.tbl");
    tbl_title3 = file_load("textes\\title3.tbl");
    tbl_title4 = file_load("textes\\title4.tbl");
    tbl_title5 = file_load("textes\\title5.tbl");
    tbl_title_secret = file_load("textes\\title_secret.tbl");

    tbl_name1 = file_load("textes\\name1.tbl");
    tbl_name2 = file_load("textes\\name2.tbl");
    tbl_name3 = file_load("textes\\name3.tbl");
    tbl_name4 = file_load("textes\\name4.tbl");
    tbl_name8 = file_load("textes\\name8.tbl");
    tbl_name10 = file_load("textes\\name10.tbl");
    tbl_name11 = file_load("textes\\name11.tbl");
    tbl_name12 = file_load("textes\\name12.tbl");
    tbl_name13 = file_load("textes\\name13.tbl");
    tbl_name14 = file_load("textes\\name14.tbl");
    tbl_name15 = file_load("textes\\name15.tbl");
    tbl_name16 = file_load("textes\\name16.tbl");
    tbl_name17 = file_load("textes\\name17.tbl");
    tbl_name18 = file_load("textes\\name18.tbl");
    tbl_name19 = file_load("textes\\name19.tbl");
    tbl_name20 = file_load("textes\\name20.tbl");
    tbl_name21 = file_load("textes\\name21.tbl");
    tbl_name22 = file_load("textes\\name22.tbl");
    tbl_name23 = file_load("textes\\name23.tbl");
    tbl_name24 = file_load("textes\\name24.tbl");
    tbl_name25 = file_load("textes\\name25.tbl");
    tbl_name26 = file_load("textes\\name26.tbl");
    tbl_name27 = file_load("textes\\name27.tbl");
    tbl_name28 = file_load("textes\\name28.tbl");
    tbl_name29 = file_load("textes\\name29.tbl");
    tbl_name30 = file_load("textes\\name30.tbl");
    tbl_name31 = file_load("textes\\name31.tbl");
    tbl_name32 = file_load("textes\\name32.tbl");
    tbl_name33 = file_load("textes\\name33.tbl");
    tbl_name34 = file_load("textes\\name34.tbl");
    tbl_name35 = file_load("textes\\name35.tbl");

    tbl_names_secret1 = file_load("textes\\names_secret1.tbl");
    tbl_names_secret2 = file_load("textes\\names_secret2.tbl");
    tbl_names_secret3 = file_load("textes\\names_secret3.tbl");

    tbl_skill1 = file_load("textes\\skill1.tbl");
    tbl_skill2 = file_load("textes\\skill2.tbl");
    tbl_skill3 = file_load("textes\\skill3.tbl");
    tbl_skill4 = file_load("textes\\skill4.tbl");

    tbl_research1 = file_load("textes\\research1.tbl");
    tbl_research2 = file_load("textes\\research2.tbl");
    tbl_research3 = file_load("textes\\research3.tbl");
    tbl_research4 = file_load("textes\\research4.tbl");
    tbl_research5 = file_load("textes\\research5.tbl");
    tbl_research6 = file_load("textes\\research6.tbl");
    tbl_research7 = file_load("textes\\research7.tbl");
    tbl_research8 = file_load("textes\\research8.tbl");
    tbl_research9 = file_load("textes\\research9.tbl");
    tbl_research10 = file_load("textes\\research10.tbl");
    tbl_research11 = file_load("textes\\research11.tbl");

    tbl_train1 = file_load("textes\\train1.tbl");
    tbl_train2 = file_load("textes\\train2.tbl");
    tbl_train3 = file_load("textes\\train3.tbl");
    tbl_train4 = file_load("textes\\train4.tbl");
    tbl_train5 = file_load("textes\\train5.tbl");
    tbl_train6 = file_load("textes\\train6.tbl");

    tbl_build1 = file_load("textes\\build1.tbl");
    tbl_build2 = file_load("textes\\build2.tbl");
    tbl_build3 = file_load("textes\\build3.tbl");
    tbl_build4 = file_load("textes\\build4.tbl");
    tbl_build5 = file_load("textes\\build5.tbl");

    tbl_nations = file_load("textes\\nations.tbl");

    tbl_error1 = file_load("textes\\error1.tbl");

    tbl_reinforcements = file_load("textes\\reinforcements.tbl");

    file_load_size("maps\\map1.pud", &pud_map1, &pud_map1_size);
    file_load_size("maps\\map2.pud", &pud_map2, &pud_map2_size);
    file_load_size("maps\\map3.pud", &pud_map3, &pud_map3_size);
    file_load_size("maps\\map4.pud", &pud_map4, &pud_map4_size);
    file_load_size("maps\\map5.pud", &pud_map5, &pud_map5_size);
    file_load_size("maps\\emap1.pud", &pud_emap1, &pud_emap1_size);
    file_load_size("maps\\emap2.pud", &pud_emap2, &pud_emap2_size);
    file_load_size("maps\\emap3.pud", &pud_emap3, &pud_emap3_size);
    file_load_size("maps\\emap4.pud", &pud_emap4, &pud_emap4_size);
    file_load_size("maps\\emap5.pud", &pud_emap5, &pud_emap5_size);
    file_load_size("maps\\map_secret.pud", &pud_map_secret, &pud_map_secret_size);

    file_load_size("bin\\menu.bin", &bin_menu, &bin_menu_size);
    file_load_size("bin\\menu.bin", &bin_menu_copy, &bin_menu_size);
    file_load_size("bin\\sngl.bin", &bin_sngl, &bin_sngl_size);
    file_load_size("bin\\sngl.bin", &bin_sngl_copy, &bin_sngl_size);
    file_load_size("bin\\newcmp.bin", &bin_newcmp, &bin_newcmp_size);
    file_load_size("bin\\newcmp.bin", &bin_newcmp_copy, &bin_newcmp_size);
    file_load_size("bin\\newcmp2.bin", &bin_newcmp2, &bin_newcmp2_size);
    file_load_size("bin\\newcmp2.bin", &bin_newcmp2_copy, &bin_newcmp2_size);
    file_load_size("bin\\quit.bin", &bin_quit, &bin_quit_size);
    file_load_size("bin\\quit.bin", &bin_quit_copy, &bin_quit_size);
    file_load_size("bin\\hdisptch.bin", &bin_hdisptch, &bin_hdisptch_size);
    file_load_size("bin\\hdisptch.bin", &bin_hdisptch_copy, &bin_hdisptch_size);
    file_load_size("bin\\credits.bin", &bin_credits, &bin_credits_size);
    file_load_size("bin\\credits.bin", &bin_credits_copy, &bin_credits_size);

    file_load_size("bin\\ai.bin", &bin_AI, &bin_AI_size);
    file_load_size("bin\\script.bin", &bin_script, &bin_script_size);
    file_load_size("bin\\unitdata.dat", &bin_unitdata, &bin_unitdata_size);
    file_load_size("bin\\unitdato.dat", &bin_unitdato, &bin_unitdato_size);
    file_load_size("bin\\upgrades.dat", &bin_upgrades, &bin_upgrades_size);

    file_load_size("tilesets\\desert.cv4", &tile_desert_cv4, &tile_desert_cv4_size);
    file_load_size("tilesets\\desert.vr4", &tile_desert_vr4, &tile_desert_vr4_size);
    file_load_size("tilesets\\desert.vx4", &tile_desert_vx4, &tile_desert_vx4_size);
    tile_desert_palette = file_load("tilesets\\desert.ppl");
    file_load_size("tilesets\\iceland.cv4", &tile_iceland_cv4, &tile_iceland_cv4_size);
    file_load_size("tilesets\\iceland.vr4", &tile_iceland_vr4, &tile_iceland_vr4_size);
    file_load_size("tilesets\\iceland.vx4", &tile_iceland_vx4, &tile_iceland_vx4_size);
    tile_iceland_palette = file_load("tilesets\\iceland.ppl");

    pcx_end = file_load("images\\end.raw");
    pcx_end_pal = file_load("images\\end.pal");
    pcx_end2 = file_load("images\\end2.raw");
    pcx_end2_pal = file_load("images\\end2.pal");
    pcx_credits = file_load("images\\credits.raw");
    pcx_credits_pal = file_load("images\\credits.pal");
    pcx_act1 = file_load("images\\act1.raw");
    pcx_act1_pal = file_load("images\\act1.pal");
    pcx_act2 = file_load("images\\act2.raw");
    pcx_act2_pal = file_load("images\\act2.pal");
    pcx_act3 = file_load("images\\act3.raw");
    pcx_act3_pal = file_load("images\\act3.pal");
    pcx_act4 = file_load("images\\act4.raw");
    pcx_act4_pal = file_load("images\\act4.pal");
    pcx_act5 = file_load("images\\act5.raw");
    pcx_act5_pal = file_load("images\\act5.pal");

    pcx_act_secret = file_load("images\\act_secret.raw");
    pcx_act_secret_pal = file_load("images\\act_secret.pal");

    pcx_brif1 = file_load("images\\brif1.raw");
    pcx_brif1_pal = file_load("images\\brif1.pal");
    pcx_brif2 = file_load("images\\brif2.raw");
    pcx_brif2_pal = file_load("images\\brif2.pal");
    pcx_brif3 = file_load("images\\brif3.raw");
    pcx_brif3_pal = file_load("images\\brif3.pal");
    pcx_brif4 = file_load("images\\brif4.raw");
    pcx_brif4_pal = file_load("images\\brif4.pal");
    pcx_brif5 = file_load("images\\brif5.raw");
    pcx_brif5_pal = file_load("images\\brif5.pal");
    pcx_brif_secret = file_load("images\\brif_secret.raw");
    pcx_brif_secret_pal = file_load("images\\brif_secret.pal");

    pcx_win = file_load("images\\win.raw");
    pcx_win_pal = file_load("images\\win.pal");
    pcx_loss = file_load("images\\loss.raw");
    pcx_loss_pal = file_load("images\\loss.pal");	
}

PROC g_proc_0042A443;
void act_init()
{
    WORD m = *(WORD *)LEVEL_ID;
    *(WORD *)LEVEL_ID = 0x52C8;  // mission file number
    *(WORD *)PREVIOUS_ACT = 0;       // prev act
    ((void (*)())g_proc_0042A443)(); // original
    *(WORD *)LEVEL_ID = m;           // mission file number restore

    if (*(byte*)LEVEL_OBJ == LVL_ORC1)*(byte*)PLAYER_RACE = 1;
    else *(byte*)PLAYER_RACE = 0;// human = 0
}

PROC g_proc_00422D76;
void sound_play_unit_speech_replace(WORD sid, int a, int *u, int b, void* &snd, char *name)
{
    def_name = (void *)*(int *)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * sid); // save default
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)name);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)snd);
    ((void (*)(WORD, int, int *, int))g_proc_00422D76)(sid, a, u, b); // original
    snd = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)def_sound);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name); // restore default
}

// PROC g_proc_00422D76;
void sound_play_unit_speech(WORD sid, int a, int *u, int b)
{
    bool f = true;
    if (u != NULL)
    {
        WORD sn = 0;
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if ((sid >= 262) && (sid <= 270))
            {
                sn = sid - 262;
                sound_play_unit_speech_replace(sid, a, u, b, h_sounds[sn], h_names[sn]);
                f = false;
            }
        }
        else if (id == U_STABLES)
        {
            if (sid == 49)
            {
                sound_play_unit_speech_replace(sid, a, u, b, assembly_sound, assembly_name);
                f = false;
            }
        }
        else if (id == U_FARM)
        {
            if (sid == 50)
            {
                sound_play_unit_speech_replace(sid, a, u, b, farm_sound, farm_name);
                f = false;
            }
        }
        else if (id == U_CHURCH)
        {
            if (sid == 48)
            {
                sound_play_unit_speech_replace(sid, a, u, b, church_sound, church_name);
                f = false;
            }
        }
        else if ((id == U_PEASANT) || (id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if ((sid >= 223) && (sid <= 238)) // peasant
            {
                sn = sid - 223;
                sound_play_unit_speech_replace(sid, a, u, b, w_sounds[sn], w_names[sn]);
                f = false;
            }
            else if (sid == 21) // research
            {
                sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                f = false;
            }
            if ((sid >= 262) && (sid <= 270)) // danath
            {
                sn = sid - 262;
                sound_play_unit_speech_replace(sid, a, u, b, h_sounds[sn], h_names[sn]);
                f = false;
            }
        }
        else if ((id == U_HBARRACK) || (id == U_FOOTMAN) || (id == U_ARCHER) || (id == U_RANGER) || (id == U_KNIGHT) || (id == U_PALADIN))
        {
            if (sid == 21) // research
            {
                sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                f = false;
            }
            else if ((sid >= 3) && (sid <= 23)) // footman
            {
                sn = sid - 3;
                sound_play_unit_speech_replace(sid, a, u, b, hu_sounds[sn], hu_names[sn]);
                f = false;
            }
            else if ((sid >= 106) && (sid <= 117)) // elf
            {
                sn = sid - 106;
                sound_play_unit_speech_replace(sid, a, u, b, e_sounds[sn], e_names[sn]);
                f = false;
            }
            else if ((sid >= 141) && (sid <= 152)) // knight
            {
                sn = sid - 141;
                sound_play_unit_speech_replace(sid, a, u, b, k_sounds[sn], k_names[sn]);
                f = false;
            }
            else if ((sid >= 153) && (sid <= 164)) // paladin
            {
                sn = sid - 153;
                sound_play_unit_speech_replace(sid, a, u, b, pk_sounds[sn], pk_names[sn]);
                f = false;
            }
        }
        else if ((id == U_INVENTOR) || (id == U_DWARWES) || (id == U_FLYER))
        {
            if ((sid >= 95) && (sid <= 105)) // dwarf
            {
                sn = sid - 95;
                sound_play_unit_speech_replace(sid, a, u, b, d_sounds[sn], d_names[sn]);
                f = false;
            }
            else if ((sid >= 118) && (sid <= 124)) // gnome
            {
                sn = sid - 118;
                sound_play_unit_speech_replace(sid, a, u, b, g_sounds[sn], g_names[sn]);
                f = false;
            }
        }
        else if ((id == U_SHIPYARD) || (id == U_HTANKER) || (id == U_HDESTROYER) || (id == U_BATTLESHIP) || (id == U_HTRANSPORT) || (id == U_SUBMARINE))
        {
            if ((sid >= 189) && (sid <= 198)) // ships
            {
                sn = sid - 189;
                sound_play_unit_speech_replace(sid, a, u, b, s_sounds[sn], s_names[sn]);
                f = false;
            }
        }
        if ((sid == 26) || (sid == 27))
        {
            bool f2 = false;
            if (id == U_PEASANT)
                f2 = true;
            else if (id == U_FOOTMAN)
                f2 = true;
            else if (id == U_KNIGHT)
                f2 = true;
            else if (id == U_DANATH)
                f2 = true;
            else if (id == U_MAGE)
                f2 = true;
            else if (id == U_ATTACK_PEASANT)
                f2 = true;
            if (f2)
            {
                sound_play_unit_speech_replace(sid, a, u, b, dd_sound, dd_name);
                f = false;
            }
        }
    }
    if (f)
    {
        if (sid == 22)
        {
            sound_play_unit_speech_replace(sid, a, u, b, wd_sound, wd_name);
            f = false;
        }
        else if (sid == 21)
        {
            sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
            f = false;
        }
        else if (sid == 24)
        {
            sound_play_unit_speech_replace(sid, a, u, b, hh1_sound, hh1_name);
            f = false;
        }
        else if (sid == 25)
        {
            sound_play_unit_speech_replace(sid, a, u, b, hh2_sound, hh2_name);
            f = false;
        }
    }
    if (f)
        ((void (*)(WORD, int, int *, int))g_proc_00422D76)(sid, a, u, b); // original
}

PROC g_proc_00422D5F;
void sound_play_unit_speech_soft_replace(WORD sid, int a, int* u, int b, void* &snd, char* name)
{
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)snd);
    ((void (*)(WORD, int, int*, int))g_proc_00422D5F)(sid, a, u, b);//original
    snd = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name);//restore default
}

//PROC g_proc_00422D5F;
void sound_play_unit_speech_soft(WORD sid, int a, int *u, int b)
{
    bool f = true;
    if (u != NULL)
    {
        WORD sn = 0;
        byte id = *((byte *)((uintptr_t)u + S_ID));
        if ((sid == 26) || (sid == 27))
        {
            bool f2 = false;
            if (id == U_PEASANT)
                f2 = true;
            else if (id == U_FOOTMAN)
                f2 = true;
            else if (id == U_KNIGHT)
                f2 = true;
            else if (id == U_DANATH)
                f2 = true;
            else if (id == U_ATTACK_PEASANT)
                f2 = true;
            if (f2)
            {
                sound_play_unit_speech_soft_replace(sid, a, u, b, dd_sound, dd_name);
                f = false;
            }
        }
    }
    if (f)
        ((void (*)(WORD, int, int *, int))g_proc_00422D5F)(sid, a, u, b); // original
}

//-------------files
PROC g_proc_0044F37D;
void main_menu_init(int a)
{
    if (remember_music != 101)
        *(DWORD *)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD *)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM); // set volume
    remember_music = 101;
    remember_sound = 101;

    *(byte *)PLAYER_RACE = 0; // human cursor

    game_started = false;

    ((void (*)(int))g_proc_0044F37D)(a);
}

PROC g_proc_00418937;
void dispatch_die_unitdraw_update_1_man(int *u)
{
    ((void (*)(int *))g_proc_00418937)(u); // original
    bool f = false;
    byte id = *((byte *)((uintptr_t)u + S_ID));
    if (id == U_PEASANT)
        f = true;
    if (id == U_FOOTMAN)
        f = true;
    if (id == U_KNIGHT)
        f = true;
    if (id == U_DANATH)
        f = true;
    if (id == U_ATTACK_PEASANT)
        f = true;
    if (f)
        set_stat(u, 100, S_MOVEMENT_TYPE);
    if (id == U_PEON)
    {
        set_stat(u, 0, S_PEON_FLAGS);
    }
}

PROC g_proc_00451590;
void unit_kill_peon_change(int* u)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id == U_PEON)
    {
        byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
        pf &= ~PEON_LOADED;
        set_stat(u, pf, S_PEON_FLAGS);
    }
    ((void (*)(int*))g_proc_00451590)(u);//original
}

PROC g_proc_0042BB04;
int *map_create_unit(int x, int y, byte id, byte o)
{
    int *u = NULL;
    u = ((int *(*)(int, int, byte, byte))g_proc_0042BB04)(x, y, id, o);
    if (u != NULL)
    {
        if (ai_fixed)
        {
            if ((id == U_PEASANT) || (id == U_PEON))
            {
                set_stat(u, 255, S_NEXT_FIRE);
                set_stat(u, 2, S_NEXT_FIRE + 1);
            }
        }
    }
    return u;
}

PROC g_proc_00424745; // entering
PROC g_proc_004529C0; // grow struct
int goods_into_inventory(int *p)
{
    int tr = (*(int *)((uintptr_t)p + S_ORDER_UNIT_POINTER));
    if (tr != 0)
    {
        bool f = false;
        int *trg = (int *)tr;
        byte o = *((byte *)((uintptr_t)p + S_OWNER));
        byte id = *((byte *)((uintptr_t)p + S_ID));
        byte tid = *((byte *)((uintptr_t)trg + S_ID));
        byte pf = *((byte *)((uintptr_t)p + S_PEON_FLAGS));
        int pflag = *(int *)(UNIT_GLOBAL_FLAGS + id * 4);
        int tflag = *(int *)(UNIT_GLOBAL_FLAGS + tid * 4);
        int res = 100;

        find_all_alive_units(U_CHURCH);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_complete();
        int market = units;

        if (pf & PEON_LOADED)
        {
            if (((pflag & IS_SHIP) != 0) && ((tflag & IS_OILRIG) == 0))
            {
                int r = get_val(REFINERY, o);
                if (r != 0)
                    res = 125;
                else
                    res = 100;
                change_res(o, 2, 1, res);
                add_total_res(o, 2, 1, res);
                f = true;
            }
            else
            {
                if (((tflag & IS_TOWNHALL) != 0) || ((tflag & IS_LUMBER) != 0))
                {
                    if (((tflag & IS_TOWNHALL) != 0))
                    {
                        pf |= PEON_IN_CASTLE;
                        set_stat(p, pf, S_PEON_FLAGS);
                    }
                    if (((pf & PEON_HARVEST_GOLD) != 0) && ((tflag & IS_TOWNHALL) != 0))
                    {
                        int r2 = get_val(TH2, o);
                        int r3 = get_val(TH3, o);
                        if (r3 != 0)
                            res = 120;
                        else
                        {
                            if (r2 != 0)
                                res = 110;
                            else
                                res = 100;
                        }
                        if (o == *(byte*)LOCAL_PLAYER)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_HASTE)) != 0)
                                res += 10;
                            res += 5 * market;
                        }
                        pf &= ~PEON_HARVEST_GOLD;
                        change_res(o, 0, 1, res);
                        add_total_res(o, 0, 1, res);
                        f = true;
                    }
                    else
                    {
                        if (((pf & PEON_HARVEST_LUMBER) != 0))
                        {
                            int r = get_val(LUMBERMILL, o);
                            if (r != 0)
                                res = 125;
                            else
                                res = 100;
                            if (o == *(byte*)LOCAL_PLAYER)
                            {
                                if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_DD)) != 0)
                                    res += 25;
                                res += 5 * market;
                            }
                            pf &= ~PEON_HARVEST_LUMBER;
                            change_res(o, 1, 1, res);
                            add_total_res(o, 1, 1, res);
                            f = true;
                        }
                    }
                }
            }
        }
        if (f)
        {
            pf &= ~PEON_LOADED;
            set_stat(p, pf, S_PEON_FLAGS);
            ((void (*)(int *))F_GROUP_SET)(p);
            return 1;
        }
    }
    return 0;
    // return ((int(*)(int*))g_proc_00424745)(p);//original
}

PROC g_proc_0042479E;
void peon_into_goldmine(int* u)
{
    ((void (*)(int*))g_proc_0042479E)(u); // original
    byte lvl = *(byte*)LEVEL_OBJ;
    if (lvl == LVL_HUMAN2)
    {
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            if (*(byte*)(CONTROLER_TYPE + o) == C_COMP)
            {
                int* g = (int*)*((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER));
                WORD r = *((WORD*)((uintptr_t)g + S_RESOURCES));
                if (r < 5)r++;
                set_stat(g, r, S_RESOURCES);
            }
    }
}

PROC g_proc_00437679;
int in_range(int* u, byte ord)
{
    if (ord == ORDER_REPAIR)
    {
        int* t = (int*)*((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER));
        byte tid = *((byte*)((uintptr_t)t + S_ID));
        bool f = (tid == U_HTANKER) || (tid == U_OTANKER) || (tid == U_HDESTROYER) || (tid == U_ODESTROYER) ||
            (tid == U_BATTLESHIP) || (tid == U_JUGGERNAUT) || (tid == U_SUBMARINE) || (tid == U_TURTLE);
        if (f)
        {
            char bufr[] = "\x2";
            PATCH_SET((char*)(0x004962B0 + ORDER_REPAIR), bufr);//004962B0 order dist table 1b  (255 - no range, 254 - unit range)
        }
    }
    int original = ((int (*)(int*, byte))g_proc_00437679)(u, ord);//original
    char bufr[] = "\x1";
    PATCH_SET((char*)(0x004962B0 + ORDER_REPAIR), bufr);
    return original;
}

PROC g_proc_0042A466;
void briefing_check()
{
    *(byte *)(LEVEL_OBJ + 1) = 0;
    ((void (*)())g_proc_0042A466)(); // original
}

PROC g_proc_00416930;
void player_race_mission_cheat()
{
    if (*(byte*)LEVEL_OBJ == LVL_ORC1)*(byte*)PLAYER_RACE = 1;
    else *(byte*)PLAYER_RACE = 0;// human = 0
    ((void (*)())g_proc_00416930)(); // original
}

PROC g_proc_0042AC6D;
void player_race_mission_cheat2()
{
    ((void (*)())g_proc_0042AC6D)(); // original
    if (*(byte*)LEVEL_OBJ == LVL_ORC1)*(byte*)PLAYER_RACE = 1;
    else *(byte *)PLAYER_RACE = 0;// human = 0
}

void sounds_ready_table_set(byte id, WORD snd)
{
    char buf[] = "\x0\x0";
    buf[0] = snd % 256;
    buf[1] = snd / 256;
    PATCH_SET((char *)(UNIT_SOUNDS_READY_TABLE + 2 * id), buf);
}

void sounds_tables()
{
    sounds_ready_table_set(U_DANATH, 263);
}

#define BUTTONS_MAX 30

char buttons_hbarak[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_hbarak_orig;
DWORD buttons_hbarak_orig_amount;

char buttons_th[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_th_orig;
DWORD buttons_th_orig_amount;

char buttons_church[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_church_orig;
DWORD buttons_church_orig_amount;

char buttons_hsmith[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_hsmith_orig;
DWORD buttons_hsmith_orig_amount;

char buttons_hlumber[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_hlumber_orig;
DWORD buttons_hlumber_orig_amount;

char buttons_hfoundry[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_hfoundry_orig;
DWORD buttons_hfoundry_orig_amount;

char buttons_shipyard[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_shipyard_orig;
DWORD buttons_shipyard_orig_amount;

char buttons_inventor[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_inventor_orig;
DWORD buttons_inventor_orig_amount;

char buttons_aviary[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_aviary_orig;
DWORD buttons_aviary_orig_amount;

char buttons_magetower[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_magetower_orig;
DWORD buttons_magetower_orig_amount;

char buttons_htower[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_htower_orig;
DWORD buttons_htower_orig_amount;

char buttons_obarak[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_obarak_orig;
DWORD buttons_obarak_orig_amount;

char buttons_gh[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_gh_orig;
DWORD buttons_gh_orig_amount;

char buttons_altar[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_altar_orig;
DWORD buttons_altar_orig_amount;

char buttons_osmith[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_osmith_orig;
DWORD buttons_osmith_orig_amount;

char buttons_olumber[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_olumber_orig;
DWORD buttons_olumber_orig_amount;

char buttons_ofoundry[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_ofoundry_orig;
DWORD buttons_ofoundry_orig_amount;

char buttons_wharf[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_wharf_orig;
DWORD buttons_wharf_orig_amount;

char buttons_alchemist[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_alchemist_orig;
DWORD buttons_alchemist_orig_amount;

char buttons_roost[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_roost_orig;
DWORD buttons_roost_orig_amount;

char buttons_temple[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_temple_orig;
DWORD buttons_temple_orig_amount;

char buttons_otower[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_otower_orig;
DWORD buttons_otower_orig_amount;

//--------------------------------------
char buttons_peasant[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_peasant_orig;
DWORD buttons_peasant_orig_amount;

char buttons_abuild[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_abuild_orig;
DWORD buttons_abuild_orig_amount;

char buttons_build[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_build_orig;
DWORD buttons_build_orig_amount;

char buttons_hero[BUTTONS_MAX * BUTTON_SIZE + 1];
DWORD buttons_hero_orig;
DWORD buttons_hero_orig_amount;

char buttons_farm[BUTTONS_MAX * BUTTON_SIZE + 1];

char buttons_stables[BUTTONS_MAX * BUTTON_SIZE + 1];

void empty_build_cancel(int id)
{
    int* p = NULL;
    for (int i = 0; ((i < 9) && (p == NULL)); i++)
    {
        p = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (p)break;
    }
    if (p)
    {
        byte ow = *((byte*)((uintptr_t)p + S_OWNER));
        if (ow == *(byte*)LOCAL_PLAYER)
        {
            byte id = *((byte*)((uintptr_t)p + S_ID));
            if (id >= U_FARM)
            {
                WORD uid = unit_fixup(p);
                int k = 0;
                int kk = -1;
                while (k <= 7)
                {
                    if (build_queue[uid * 16 + k * 2] != 255)kk = k;
                    k++;
                }
                if (kk != -1)
                {
                    build_queue[uid * 16 + kk * 2] = 255;
                    build_queue[uid * 16 + kk * 2 + 1] = 255;
                    return;
                }
            }
        }
    }
    ((void (*)(int))0x00436180)(id);//orig cancel
}

void make_buttons_copy_from_orig(char* buttons_array, DWORD buttons_orig_adress, DWORD amount)
{
    for (int i = 0; i < BUTTON_SIZE * amount; i++)
    {
        buttons_array[i] = *(byte*)(buttons_orig_adress + i);
    }
}

void make_button(char* buttons_array, int k, byte id, WORD icon, int (*check_function) (byte), void (*action_function) (int), byte check_arg, byte action_arg, WORD stat_txt)
{
    buttons_array[BUTTON_SIZE * k + 0] = (id > 8) ? 8 : id;//button id?
    buttons_array[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_array[BUTTON_SIZE * k + 2] = (byte)(icon % 256);//icon
    buttons_array[BUTTON_SIZE * k + 3] = (byte)(icon / 256);//icon
    patch_setdword((DWORD*)(buttons_array + (BUTTON_SIZE * k + 4)), (DWORD)check_function);
    patch_setdword((DWORD*)(buttons_array + (BUTTON_SIZE * k + 8)), (DWORD)action_function);
    buttons_array[BUTTON_SIZE * k + 12] = check_arg;//arg
    buttons_array[BUTTON_SIZE * k + 13] = action_arg;//unit id
    buttons_array[BUTTON_SIZE * k + 14] = (byte)(stat_txt % 256);//string from tbl
    buttons_array[BUTTON_SIZE * k + 15] = (byte)(stat_txt / 256);//string from tbl
    buttons_array[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_array[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_array[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_array[BUTTON_SIZE * k + 19] = '\x0';//flags?
}

void make_cancel_button(char* buttons_array, int k)
{
    make_button(buttons_array, k, 8, 0x5B, empty_true, empty_build_cancel, 0, 0, 0x199);
}

void make_always_disable_orig_button(char* buttons_array, int k)
{
    patch_setdword((DWORD*)(buttons_array + (BUTTON_SIZE * k + 4)), (DWORD)empty_false);
}

void make_always_enable_orig_button(char* buttons_array, int k)
{
    patch_setdword((DWORD*)(buttons_array + (BUTTON_SIZE * k + 4)), (DWORD)empty_true);
}

void make_orig_button_change_description(char* buttons_array, int k, DWORD d)
{
    patch_setdword((DWORD*)(buttons_array + (BUTTON_SIZE * k + 14)), d);
}

void set_new_buttons(byte uid, char* buttons_array, DWORD amount)
{
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * uid), (DWORD)amount);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * uid + 4), (DWORD)buttons_array);
}

void buttons_init_hbarak()
{
    make_buttons_copy_from_orig(buttons_hbarak, buttons_hbarak_orig, buttons_hbarak_orig_amount);
    make_cancel_button(buttons_hbarak, buttons_hbarak_orig_amount);
    set_new_buttons(U_HBARRACK, buttons_hbarak, buttons_hbarak_orig_amount + 1);
}

void buttons_init_th()
{
    make_buttons_copy_from_orig(buttons_th, buttons_th_orig, buttons_th_orig_amount);
    make_cancel_button(buttons_th, buttons_th_orig_amount);
    set_new_buttons(U_TOWN_HALL, buttons_th, buttons_th_orig_amount + 1);
    set_new_buttons(U_KEEP, buttons_th, buttons_th_orig_amount + 1);
    set_new_buttons(U_CASTLE, buttons_th, buttons_th_orig_amount + 1);
}

void buttons_init_church()
{
    make_buttons_copy_from_orig(buttons_church, buttons_church_orig, buttons_church_orig_amount);
    make_cancel_button(buttons_church, buttons_church_orig_amount);
    set_new_buttons(U_CHURCH, buttons_church, buttons_church_orig_amount + 1);
}

void buttons_init_hsmith()
{
    make_buttons_copy_from_orig(buttons_hsmith, buttons_hsmith_orig, buttons_hsmith_orig_amount);
    make_cancel_button(buttons_hsmith, buttons_hsmith_orig_amount);
    set_new_buttons(U_HSMITH, buttons_hsmith, buttons_hsmith_orig_amount + 1);
}

void buttons_init_hlumber()
{
    make_buttons_copy_from_orig(buttons_hlumber, buttons_hlumber_orig, buttons_hlumber_orig_amount);
    make_cancel_button(buttons_hlumber, buttons_hlumber_orig_amount);
    set_new_buttons(U_HLUMBER, buttons_hlumber, buttons_hlumber_orig_amount + 1);
}

void buttons_init_hfoundry()
{
    make_buttons_copy_from_orig(buttons_hfoundry, buttons_hfoundry_orig, buttons_hfoundry_orig_amount);
    make_cancel_button(buttons_hfoundry, buttons_hfoundry_orig_amount);
    set_new_buttons(U_HFOUNDRY, buttons_hfoundry, buttons_hfoundry_orig_amount + 1);
}

void buttons_init_shipyard()
{
    make_buttons_copy_from_orig(buttons_shipyard, buttons_shipyard_orig, buttons_shipyard_orig_amount);
    make_cancel_button(buttons_shipyard, buttons_shipyard_orig_amount);
    set_new_buttons(U_SHIPYARD, buttons_shipyard, buttons_shipyard_orig_amount + 1);
}

void buttons_init_inventor()
{
    make_buttons_copy_from_orig(buttons_inventor, buttons_inventor_orig, buttons_inventor_orig_amount);
    make_cancel_button(buttons_inventor, buttons_inventor_orig_amount);
    set_new_buttons(U_INVENTOR, buttons_inventor, buttons_inventor_orig_amount + 1);
}

void buttons_init_aviary()
{
    make_buttons_copy_from_orig(buttons_aviary, buttons_aviary_orig, buttons_aviary_orig_amount);
    make_cancel_button(buttons_aviary, buttons_aviary_orig_amount);
    set_new_buttons(U_AVIARY, buttons_aviary, buttons_aviary_orig_amount + 1);
}

void buttons_init_magetower()
{
    make_buttons_copy_from_orig(buttons_magetower, buttons_magetower_orig, buttons_magetower_orig_amount);
    make_cancel_button(buttons_magetower, buttons_magetower_orig_amount);
    set_new_buttons(U_MAGE_TOWER, buttons_magetower, buttons_magetower_orig_amount + 1);
}

void buttons_init_htower()
{
    make_buttons_copy_from_orig(buttons_htower, buttons_htower_orig, buttons_htower_orig_amount);
    make_cancel_button(buttons_htower, buttons_htower_orig_amount);
    set_new_buttons(U_HTOWER, buttons_htower, buttons_htower_orig_amount + 1);
}

void buttons_init_obarak()
{
    make_buttons_copy_from_orig(buttons_obarak, buttons_obarak_orig, buttons_obarak_orig_amount);
    make_cancel_button(buttons_obarak, buttons_obarak_orig_amount);
    set_new_buttons(U_OBARRACK, buttons_obarak, buttons_obarak_orig_amount + 1);
}

void buttons_init_gh()
{
    make_buttons_copy_from_orig(buttons_gh, buttons_gh_orig, buttons_gh_orig_amount);
    make_cancel_button(buttons_gh, buttons_gh_orig_amount);
    set_new_buttons(U_GREAT_HALL, buttons_gh, buttons_gh_orig_amount + 1);
    set_new_buttons(U_STRONGHOLD, buttons_gh, buttons_gh_orig_amount + 1);
    set_new_buttons(U_FORTRESS, buttons_gh, buttons_gh_orig_amount + 1);
}

void buttons_init_altar()
{
    make_buttons_copy_from_orig(buttons_altar, buttons_altar_orig, buttons_altar_orig_amount);
    make_cancel_button(buttons_altar, buttons_altar_orig_amount);
    set_new_buttons(U_ALTAR, buttons_altar, buttons_altar_orig_amount + 1);
}

void buttons_init_osmith()
{
    make_buttons_copy_from_orig(buttons_osmith, buttons_osmith_orig, buttons_osmith_orig_amount);
    make_cancel_button(buttons_osmith, buttons_osmith_orig_amount);
    set_new_buttons(U_OSMITH, buttons_osmith, buttons_osmith_orig_amount + 1);
}

void buttons_init_olumber()
{
    make_buttons_copy_from_orig(buttons_olumber, buttons_olumber_orig, buttons_olumber_orig_amount);
    make_cancel_button(buttons_olumber, buttons_olumber_orig_amount);
    set_new_buttons(U_OLUMBER, buttons_olumber, buttons_olumber_orig_amount + 1);
}

void buttons_init_ofoundry()
{
    make_buttons_copy_from_orig(buttons_ofoundry, buttons_ofoundry_orig, buttons_ofoundry_orig_amount);
    make_cancel_button(buttons_ofoundry, buttons_ofoundry_orig_amount);
    set_new_buttons(U_OFOUNDRY, buttons_ofoundry, buttons_ofoundry_orig_amount + 1);
}

void buttons_init_wharf()
{
    make_buttons_copy_from_orig(buttons_wharf, buttons_wharf_orig, buttons_wharf_orig_amount);
    make_cancel_button(buttons_wharf, buttons_wharf_orig_amount);
    set_new_buttons(U_WHARF, buttons_wharf, buttons_wharf_orig_amount + 1);
}

void buttons_init_alchemist()
{
    make_buttons_copy_from_orig(buttons_alchemist, buttons_alchemist_orig, buttons_alchemist_orig_amount);
    make_cancel_button(buttons_alchemist, buttons_alchemist_orig_amount);
    set_new_buttons(U_ALCHEMIST, buttons_alchemist, buttons_alchemist_orig_amount + 1);
}

void buttons_init_roost()
{
    make_buttons_copy_from_orig(buttons_roost, buttons_roost_orig, buttons_roost_orig_amount);
    make_cancel_button(buttons_roost, buttons_roost_orig_amount);
    set_new_buttons(U_DRAGONROOST, buttons_roost, buttons_roost_orig_amount + 1);
}

void buttons_init_temple()
{
    make_buttons_copy_from_orig(buttons_temple, buttons_temple_orig, buttons_temple_orig_amount);
    make_cancel_button(buttons_temple, buttons_temple_orig_amount);
    set_new_buttons(U_TEMPLE, buttons_temple, buttons_temple_orig_amount + 1);
}

void buttons_init_otower()
{
    make_buttons_copy_from_orig(buttons_otower, buttons_otower_orig, buttons_otower_orig_amount);
    make_cancel_button(buttons_otower, buttons_otower_orig_amount);
    set_new_buttons(U_OTOWER, buttons_otower, buttons_otower_orig_amount + 1);
}

//-------------------
void buttons_init_peasant_new()
{
    make_buttons_copy_from_orig(buttons_peasant, buttons_peasant_orig, buttons_peasant_orig_amount);
    make_always_enable_orig_button(buttons_peasant, 7);
    set_new_buttons(U_PEASANT, buttons_peasant, buttons_peasant_orig_amount);

    make_buttons_copy_from_orig(buttons_abuild, buttons_abuild_orig, buttons_abuild_orig_amount);
    make_always_disable_orig_button(buttons_abuild, 0);
    make_always_disable_orig_button(buttons_abuild, 6);
    make_always_disable_orig_button(buttons_abuild, 7);
    make_always_enable_orig_button(buttons_abuild, 5);
    make_orig_button_change_description(buttons_abuild, 3, 50);
    make_orig_button_change_description(buttons_abuild, 4, 51);
    make_orig_button_change_description(buttons_abuild, 5, 52);
    set_new_buttons(U_HUM_ABUILD_BUTTONS, buttons_abuild, buttons_abuild_orig_amount);

    make_buttons_copy_from_orig(buttons_build, buttons_build_orig, buttons_build_orig_amount);
    make_orig_button_change_description(buttons_build, 1, 53);
    make_orig_button_change_description(buttons_build, 2, 54);
    set_new_buttons(U_HUM_BUILD_BUTTONS, buttons_build, buttons_build_orig_amount);
}

int empty_check_hero(byte id)
{
    byte lvl = *(byte*)LEVEL_OBJ;
    bool m = true;
    if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
        m = false;
    if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
        m = false;
    if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
        m = false;
    if (m)
        return 0;
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                if (*((byte*)((uintptr_t)p + S_OWNER)) == *(byte*)LOCAL_PLAYER)
                {
                    byte idd = *((byte*)((uintptr_t)p + S_ID));
                    bool f = idd == id;
                    if (f)
                    {
                        if (!check_unit_dead(p))
                        {
                            // return false if player already have that unit
                            return 0;
                        }
                    }
                    f = (idd == U_TOWN_HALL) || (idd == U_GREAT_HALL) ||
                        (idd == U_STRONGHOLD) || (idd == U_KEEP) ||
                        (idd == U_CASTLE) || (idd == U_FORTRESS);
                    if (f)
                    {
                        if (!check_unit_dead(p) && check_unit_complete(p))
                        {
                            if (*((byte*)((uintptr_t)p + S_BUILD_ORDER)) == 0)
                            {
                                if (*((byte*)((uintptr_t)p + S_BUILD_TYPE)) == id)
                                {
                                    if (*((WORD*)((uintptr_t)p + S_BUILD_PROGRES)) != 0)
                                    {
                                        // return false if player already building that unit
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
    return 1;
}

void empty_cast_shield(int id)
{
    int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (uf)
    {
        byte id = *((byte*)((uintptr_t)uf + S_ID));
        if (id == U_DANATH)
        {
            byte mp = *((byte*)((uintptr_t)uf + S_MANA));
            if (mp >= 150)
            {
                mp -= 150;
                byte x = *((byte*)((uintptr_t)uf + S_X));
                byte y = *((byte*)((uintptr_t)uf + S_Y));
                set_region(x - 5, y - 5, x + 5, y + 5);
                find_all_alive_units(ANY_MEN);
                sort_stat(S_ID, U_DWARWES, CMP_NEQ);
                sort_in_region();
                for (int i = 0; i < 16; i++)
                {
                    if (!check_ally(i, *(byte*)LOCAL_PLAYER))
                        sort_stat(S_OWNER, i, CMP_NEQ);
                }
                sort_self(uf);
                set_stat_all(S_SHIELD, 666);
                set_stat(uf, mp, S_MANA);
                ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(x * 32 + 16, y * 32 + 16, SS_ARMOR);
            }
        }
    }
}

void empty_cast_bash(int id)
{
    int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (uf)
    {
        byte id = *((byte*)((uintptr_t)uf + S_ID));
        if (id == U_DANATH)
        {
            byte mp = *((byte*)((uintptr_t)uf + S_MANA));
            if (mp >= 75)
            {
                mp -= 75;
                byte x = *((byte*)((uintptr_t)uf + S_X));
                byte y = *((byte*)((uintptr_t)uf + S_Y));
                set_region(x - 3, y - 3, x + 3, y + 3);
                find_all_alive_units(ANY_MEN);
                sort_in_region();
                for (int i = 0; i < 16; i++)
                {
                    if (check_ally(i, *(byte*)LOCAL_PLAYER))
                        sort_stat(S_OWNER, i, CMP_NEQ);
                }
                set_stat_all(S_HASTE, -1000);
                damag_all(25, 0);
                bullet_create8_around_unit(uf, B_CAT_HIT);
                set_stat(uf, mp, S_MANA);
            }
        }
    }
}

void empty_nothing(int id) {}

void buttons_init_hlumber_new()
{
    make_buttons_copy_from_orig(buttons_hlumber, buttons_hlumber_orig, buttons_hlumber_orig_amount);
    make_orig_button_change_description(buttons_hlumber, 2, 3);
    make_button(buttons_hlumber, buttons_hlumber_orig_amount, 6, 1, empty_research_spells, empty_build_research_spell, A_DD, UG_SP_ROT, 1);
    make_button(buttons_hlumber, buttons_hlumber_orig_amount + 1, 7, 2, empty_research_spells, empty_build_research_spell, A_RAISE, UG_SP_RAISEDEAD, 2);
    make_cancel_button(buttons_hlumber, buttons_hlumber_orig_amount + 2);
    set_new_buttons(U_HLUMBER, buttons_hlumber, buttons_hlumber_orig_amount + 3);
}

void buttons_init_th_new()
{
    make_buttons_copy_from_orig(buttons_th, buttons_th_orig, buttons_th_orig_amount);
    make_button(buttons_th, buttons_th_orig_amount, 3, 0xBC, empty_check_hero, empty_build, U_DANATH, U_DANATH, 1);
    make_button(buttons_th, buttons_th_orig_amount + 1, 4, 1, empty_research_spells, empty_build_research_spell, A_UNHOLY, UG_SP_ARMOR, 2);
    make_cancel_button(buttons_th, buttons_th_orig_amount + 2);
    make_orig_button_change_description(buttons_th, 0, 3);
    make_orig_button_change_description(buttons_th, 1, 4);
    make_always_disable_orig_button(buttons_th, 2);
    set_new_buttons(U_TOWN_HALL, buttons_th, buttons_th_orig_amount + 3);
    set_new_buttons(U_KEEP, buttons_th, buttons_th_orig_amount + 3);
    set_new_buttons(U_CASTLE, buttons_th, buttons_th_orig_amount + 3);
}

void buttons_init_hero()
{
    make_buttons_copy_from_orig(buttons_hero, buttons_hero_orig, buttons_hero_orig_amount);
    make_button(buttons_hero, buttons_hero_orig_amount, 6, 3, empty_true, empty_cast_bash, 0, 0, 3);
    make_button(buttons_hero, buttons_hero_orig_amount + 1, 7, 4, empty_true, empty_cast_shield, 0, 0, 1);
    make_button(buttons_hero, buttons_hero_orig_amount + 2, 8, 0x71, empty_true, empty_nothing, 0, 0, 2);
    set_new_buttons(U_DANATH, buttons_hero, buttons_hero_orig_amount + 3);
}

void buttons_init_farm()
{
    make_button(buttons_farm, 0, 0, 4, empty_research_spells, empty_build_research_spell, A_HASTE, UG_SP_HASTE, 1);
    make_cancel_button(buttons_farm, 1);
    set_new_buttons(U_FARM, buttons_farm, 2);
}

void buttons_init_stables()
{
    make_button(buttons_stables, 0, 0, 1, empty_research_spells, empty_build_research_spell, A_RUNES, UG_SP_RUNES, 1);
    make_button(buttons_stables, 1, 1, 2, empty_research_spells, empty_build_research_spell, A_BLIZZARD, UG_SP_BLIZZARD, 2);
    make_button(buttons_stables, 2, 2, 3, empty_research_spells, empty_build_research_spell, A_FLAME_SHIELD, UG_SP_FIRESHIELD, 3);
    make_cancel_button(buttons_stables, 3);
    set_new_buttons(U_STABLES, buttons_stables, 4);
}

int empty_check_tower(byte id)
{

    byte lvl = *(byte*)LEVEL_OBJ;
    bool m = true;
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1) || (lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2)
        || (lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3) || (lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
        m = false;
    int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (uf)
    {
        byte id = *((byte*)((uintptr_t)uf + S_ID));
        if (id != U_HTOWER)
        {
            m = false;
        }
    }
    if (m)if (get_val(LUMBERMILL, *(byte*)LOCAL_PLAYER) != 0)return 1;
    return 0;
}

void buttons_init_htower_new()
{
    make_buttons_copy_from_orig(buttons_htower, buttons_htower_orig, buttons_htower_orig_amount);
    patch_setdword((DWORD*)(buttons_htower + (BUTTON_SIZE * 0 + 4)), (DWORD)empty_check_tower);
    make_cancel_button(buttons_htower, buttons_htower_orig_amount);
    set_new_buttons(U_HTOWER, buttons_htower, buttons_htower_orig_amount + 1);
}

void empty_beer(int id)
{
    if (cmp_res(*(byte*)LOCAL_PLAYER, 0, 0xE8, 0x3, 0, 0, CMP_BIGGER_EQ))
    {
        sound_play_from_file(0, (DWORD)beer_name, beer_sound);
        change_res(*(byte*)LOCAL_PLAYER, 3, 1, 1000);
        find_all_alive_units(U_PEASANT);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        set_stat_all(S_HASTE, 1700);
        find_all_alive_units(U_FOOTMAN);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        set_stat_all(S_HASTE, 1700);
        find_all_alive_units(U_KNIGHT);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        set_stat_all(S_HASTE, 1700);
        find_all_alive_units(U_DWARWES);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        set_stat_all(S_HASTE, 1700);
        find_all_alive_units(U_DANATH);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        set_stat_all(S_HASTE, 1700);
    }
}

void buttons_init_church_new()
{
    make_button(buttons_church, 0, 0, 1, empty_true, empty_beer, 0, 0, 1);
    set_new_buttons(U_CHURCH, buttons_church, 1);
}

void buttons_init_hsmith_new()
{
    make_buttons_copy_from_orig(buttons_hsmith, buttons_hsmith_orig, buttons_hsmith_orig_amount);
    make_orig_button_change_description(buttons_hsmith, 4, 1);
    make_orig_button_change_description(buttons_hsmith, 5, 2);
    make_orig_button_change_description(buttons_hsmith, 0, 3);
    make_orig_button_change_description(buttons_hsmith, 1, 4);
    make_orig_button_change_description(buttons_hsmith, 2, 5);
    make_orig_button_change_description(buttons_hsmith, 3, 6);
    make_cancel_button(buttons_hsmith, buttons_hsmith_orig_amount);
    set_new_buttons(U_HSMITH, buttons_hsmith, buttons_hsmith_orig_amount + 1);
}

void buttons_init_hbarak_new()
{
    make_buttons_copy_from_orig(buttons_hbarak, buttons_hbarak_orig, buttons_hbarak_orig_amount);
    make_orig_button_change_description(buttons_hbarak, 0, 1);
    make_always_disable_orig_button(buttons_hbarak, 1);
    make_orig_button_change_description(buttons_hbarak, 3, 2);
    make_orig_button_change_description(buttons_hbarak, 4, 3);
    make_cancel_button(buttons_hbarak, buttons_hbarak_orig_amount);
    set_new_buttons(U_HBARRACK, buttons_hbarak, buttons_hbarak_orig_amount + 1);
}
//----------------------

void buttons_init_all()
{
    //buttons_init_hbarak();
    //buttons_init_th();
    //buttons_init_church();
    //buttons_init_hsmith();
    //buttons_init_hlumber();
    buttons_init_hfoundry();
    buttons_init_shipyard();
    buttons_init_inventor();
    buttons_init_aviary();
    buttons_init_magetower();
    //buttons_init_htower();
    buttons_init_obarak();
    buttons_init_gh();
    buttons_init_altar();
    buttons_init_osmith();
    buttons_init_olumber();
    buttons_init_ofoundry();
    buttons_init_wharf();
    buttons_init_alchemist();
    buttons_init_roost();
    buttons_init_temple();
    buttons_init_otower();
    //-------------
    buttons_init_peasant_new();
    buttons_init_hlumber_new();
    buttons_init_th_new();
    buttons_init_hero();
    buttons_init_farm();
    buttons_init_stables();
    buttons_init_htower_new();
    buttons_init_church_new();
    buttons_init_hsmith_new();
    buttons_init_hbarak_new();
}

void buttons_orig()
{
    buttons_hbarak_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HBARRACK);
    buttons_hbarak_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HBARRACK + 4);

    buttons_th_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_TOWN_HALL);
    buttons_th_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_TOWN_HALL + 4);

    buttons_church_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_CHURCH);
    buttons_church_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_CHURCH + 4);

    buttons_hsmith_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HSMITH);
    buttons_hsmith_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HSMITH + 4);

    buttons_hlumber_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HLUMBER);
    buttons_hlumber_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HLUMBER + 4);

    buttons_hfoundry_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HFOUNDRY);
    buttons_hfoundry_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HFOUNDRY + 4);

    buttons_shipyard_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_SHIPYARD);
    buttons_shipyard_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_SHIPYARD + 4);

    buttons_inventor_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_INVENTOR);
    buttons_inventor_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_INVENTOR + 4);

    buttons_aviary_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_AVIARY);
    buttons_aviary_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_AVIARY + 4);

    buttons_magetower_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_MAGE_TOWER);
    buttons_magetower_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_MAGE_TOWER + 4);

    buttons_htower_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HTOWER);
    buttons_htower_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HTOWER + 4);

    buttons_obarak_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_OBARRACK);
    buttons_obarak_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_OBARRACK + 4);

    buttons_gh_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_GREAT_HALL);
    buttons_gh_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_GREAT_HALL + 4);

    buttons_altar_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_ALTAR);
    buttons_altar_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_ALTAR + 4);

    buttons_osmith_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_OSMITH);
    buttons_osmith_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_OSMITH + 4);

    buttons_olumber_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_OLUMBER);
    buttons_olumber_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_OLUMBER + 4);

    buttons_ofoundry_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_OFOUNDRY);
    buttons_ofoundry_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_OFOUNDRY + 4);

    buttons_wharf_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_WHARF);
    buttons_wharf_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_WHARF + 4);

    buttons_alchemist_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_ALCHEMIST);
    buttons_alchemist_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_ALCHEMIST + 4);

    buttons_roost_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_DRAGONROOST);
    buttons_roost_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_DRAGONROOST + 4);

    buttons_temple_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_TEMPLE);
    buttons_temple_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_TEMPLE + 4);

    buttons_otower_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_OTOWER);
    buttons_otower_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_OTOWER + 4);

    //-------------------
    buttons_peasant_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_PEASANT);
    buttons_peasant_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_PEASANT + 4);

    buttons_abuild_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HUM_ABUILD_BUTTONS);
    buttons_abuild_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HUM_ABUILD_BUTTONS + 4);

    buttons_build_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_HUM_BUILD_BUTTONS);
    buttons_build_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_HUM_BUILD_BUTTONS + 4);

    buttons_hero_orig_amount = *(DWORD*)(BUTTONS_CARDS + 8 * U_DANATH);
    buttons_hero_orig = *(DWORD*)(BUTTONS_CARDS + 8 * U_DANATH + 4);
}

void buttons_not_change_status()
{
    char bufc[] = "\x90\x90\x90\x90";
    PATCH_SET((char*)0x0040E5FF, bufc);//pUnit->unitCard = CARD_CANCEL_BUILD;

    //if (gpStatUnit->uFlags & UF_BUILD_ON)
    char buf[] = "\xEB";
    PATCH_SET((char*)0x00444582, buf);//footman
    PATCH_SET((char*)0x004444B2, buf);//peon
    PATCH_SET((char*)0x004446F2, buf);//tanker
    PATCH_SET((char*)0x00444722, buf);//destroyer
    PATCH_SET((char*)0x004446C2, buf);//mage

    PATCH_SET((char*)0x004443C9, buf);//research spell

    PATCH_SET((char*)0x00443ED8, buf);//upgrade swords
    PATCH_SET((char*)0x00444138, buf);//upgrade shields
    PATCH_SET((char*)0x00443F28, buf);//upgrade cats
    PATCH_SET((char*)0x00443E68, buf);//upgrade arrows
    PATCH_SET((char*)0x004441A8, buf);//upgrade boat dmg
    PATCH_SET((char*)0x00444218, buf);//upgrade boat def


    char buf2[] = "\x90\x90";
    PATCH_SET((char*)0x004444FD, buf2);//archer
    PATCH_SET((char*)0x0044454D, buf2);//ranger
    PATCH_SET((char*)0x004445CE, buf2);//cata
    PATCH_SET((char*)0x0044461E, buf2);//knight
    PATCH_SET((char*)0x0044467E, buf2);//paladin
    PATCH_SET((char*)0x00444763, buf2);//transport
    PATCH_SET((char*)0x004447B3, buf2);//jugger
    PATCH_SET((char*)0x00444803, buf2);//sub
    PATCH_SET((char*)0x0044485E, buf2);//flyer
    PATCH_SET((char*)0x004448A3, buf2);//sap
    PATCH_SET((char*)0x004448F3, buf2);//grif

    PATCH_SET((char*)0x0044433F, buf2);//upgrade th2

    PATCH_SET((char*)0x00444433, buf2);//research spell church

    PATCH_SET((char*)0x00443F87, buf2);//upgrade rangers
    PATCH_SET((char*)0x0044404B, buf2);//upgrade scouting
    PATCH_SET((char*)0x00443FEB, buf2);//upgrade longbow
    PATCH_SET((char*)0x004440AB, buf2);//upgrade marks


    char buf3[] = "\x90\x90\x90\x90\x90\x90";
    PATCH_SET((char*)0x004442E6, buf3);//upgrade th3
}

// write your custom victory functions here
//-------------------------------------------------------------------------------
byte hero_xp = 0;

void set_random_xp_all(byte mn, byte mx)
{
    mx -= mn;
    if (mx <= 0)mx = 1;
    for (int i = 0; i < units; i++)
    {
        set_stat(unit[i], mn + rand() % mx, S_KILLS);
    }
}

void show_message_from_tbl(void* tbl, int str)
{
    char msg[MAX_PATH];
    sprintf(msg, "%s", text_from_tbl(tbl, str));
    show_message(10, msg);
}

void v_human1(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        viz_area_add(2, 72, 1 << local, 4);
		find_all_alive_units(U_DANATH);
        if (units == 0)lose(true);
        find_all_alive_units(U_PEASANT);
        sort_stat(S_OWNER, P_BLUE, CMP_EQ);
        if (units <= 1)
            lose(true);
        set_region(2, 72, 3, 73);
        sort_in_region();
        if (units >= 2)
            if ((get_val(ALTAR, local) >= 6))
            {
                find_all_alive_units(U_DANATH);
                if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
                win(true);
            }

        if (cmp_res(local, 0, 0x14, 0x5, 0, 0, CMP_SMALLER))set_res(local, 0, 0x14, 0x5, 0, 0);

        if (*(byte*)(GB_HORSES + 4) == 0)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 1)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 2);
                unit_create(119, 107, U_KNIGHT, local, 1);
                unit_create(119, 107, U_PEASANT, local, 1);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 2)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 1);
                unit_create(119, 107, U_KNIGHT, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 3)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 1);
                unit_create(119, 107, U_PEASANT, local, 1);
                unit_create(119, 107, U_BALLISTA, local, 1);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 4)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_KNIGHT, local, 2);
                unit_create(119, 107, U_DWARWES, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
    }
}

void v_human2(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

	    find_all_alive_units(ANY_UNITS);
	    sort_stat(S_OWNER, P_BLACK, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);
	
        // your custom victory conditions
        byte local = *(byte *)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        if ((get_val(FARM, local) >= 12) && (get_val(BARRACKS, local) >= 6) && (get_val(TOWER, local) >= 10) && (get_val(TH2, local) >= 1) && (get_val(ALTAR, local) >= 7))
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }
			
		if (*(byte*)(GB_HORSES + 1) == 0)
		{
			if (red_at)
			{
				change_res(P_RED, 0, 5, 1000);
				
				*(byte*)(GB_HORSES + 1) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
			}
		}

		if (*(byte*)(GB_HORSES + 2) == 0)
		{
			if (green_at)
			{
				change_res(P_GREEN, 0, 5, 1000);
				
				*(byte*)(GB_HORSES + 2) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
			}
		}

		if (*(byte*)(GB_HORSES + 3) == 0)
		{
			if (violet_at)
			{
				change_res(P_VIOLET, 0, 5, 1000);
				
				*(byte*)(GB_HORSES + 3) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
			}
		}			
    }
}

void v_human3(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions
        byte local = *(byte *)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);	
        if ((get_val(ALTAR, local) >= 6))
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }
    }
}

void v_human4(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions
        byte local = *(byte *)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        find_all_alive_units(U_DANATH);
        if (units == 0)lose(true);
        set_region(3, 99, 4, 100);
        sort_in_region();
        if (units >= 1)
        {
            *(byte*)LEVEL_OBJ = LVL_HUMAN13;
            *(WORD*)LEVEL_ID = 0x52E0;

            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }

        if (*(byte*)(GB_HORSES + 8) == 0)
        {
            find_all_alive_units(U_DK);
            if (units == 0)*(byte*)(GB_HORSES + 8) = 1;
        }
        if (*(byte*)(GB_HORSES + 8) == 1)
        {
            FILE* fp;
            if ((fp = fopen(RECORD_PATH, "a+b")) != NULL)//file opened
            {
                fclose(fp);
            }
            *(byte*)(GB_HORSES + 8) = 2;
            show_message_from_tbl(tbl_reinforcements, 1);
        }
    }
}

void v_human5(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human6(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human7(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human8(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human9(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human10(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human11(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human12(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human13(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_human14(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions

           // Союзники
        comps_vision(true); // компы могут давать виз
        ally(P_ORANGE, P_BLUE, 1);
        viz(P_ORANGE, P_BLUE, 1);
        viz(P_BLUE, P_ORANGE, 1);

        // Делаем Даната Оранжевым
        find_all_alive_units(U_DANATH);
        set_stat_all(S_COLOR, P_ORANGE);

        // Делаем союзников Оранжевыми
        find_all_alive_units(U_RANGER);
        set_stat_all(S_COLOR, P_ORANGE);

        find_all_alive_units(U_PALADIN);
        set_stat_all(S_COLOR, P_ORANGE);

        // Подкрепление
        if (*(byte*)(GB_HORSES + 14) < 40)
        {
            if (*(byte*)(GB_HORSES + 15) < 200) // таймер на 200
                *(byte*)(GB_HORSES + 15) = *(byte*)(GB_HORSES + 15) + 1;
            else
            {
                // создание подкрепления
                unit_create(127, 122, U_PALADIN, P_ORANGE, 3);
                unit_create(127, 123, U_RANGER, P_ORANGE, 3);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);

                *(byte*)(GB_HORSES + 14) += 1; // таймер 1 раз

                *(byte*)(GB_HORSES + 15) = 0; // обнуляем таймер

                // подкрепление прибывает
                find_all_alive_units(U_RANGER);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                order_all(95, 109, ORDER_MOVE);
                give_all(P_BLUE);

                find_all_alive_units(U_PALADIN);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                order_all(95, 109, ORDER_MOVE);
                give_all(P_BLUE);
            }
        }

        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        if ((get_val(ALTAR, local) >= 5))
        {
            win(true);
        }
    }
}

void v_xhuman1(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        viz_area_add(2, 72, 1 << local, 4);
        find_all_alive_units(U_DANATH);
        if (units == 0)lose(true);
        find_all_alive_units(U_PEASANT);
        sort_stat(S_OWNER, P_BLUE, CMP_EQ);
        if (units <= 1)
            lose(true);
        set_region(2, 72, 3, 73);
        sort_in_region();
        if (units >= 2)
            if ((get_val(ALTAR, local) >= 6))
            {
                find_all_alive_units(U_DANATH);
                if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
                win(true);
            }

        if (cmp_res(local, 0, 0x14, 0x5, 0, 0, CMP_SMALLER))set_res(local, 0, 0x14, 0x5, 0, 0);

        if (*(byte*)(GB_HORSES + 4) == 0)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 1)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 2);
                unit_create(119, 107, U_KNIGHT, local, 1);
                unit_create(119, 107, U_PEASANT, local, 1);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 2)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 1);
                unit_create(119, 107, U_KNIGHT, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 3)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_FOOTMAN, local, 1);
                unit_create(119, 107, U_PEASANT, local, 1);
                unit_create(119, 107, U_BALLISTA, local, 1);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
        else if (*(byte*)(GB_HORSES + 4) == 4)
        {
            if (get_val(ALTAR, local) > *(byte*)(GB_HORSES + 4))
            {
                *(byte*)(GB_HORSES + 4) = *(byte*)(GB_HORSES + 4) + 1;
                unit_create(119, 107, U_KNIGHT, local, 2);
                unit_create(119, 107, U_DWARWES, local, 2);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);
            }
        }
    }
}

void v_xhuman2(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        find_all_alive_units(ANY_UNITS);
        sort_stat(S_OWNER, P_BLACK, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);

        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        if ((get_val(FARM, local) >= 12) && (get_val(BARRACKS, local) >= 6) && (get_val(TOWER, local) >= 10) && (get_val(TH2, local) >= 1) && (get_val(ALTAR, local) >= 7))
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }

        if (*(byte*)(GB_HORSES + 1) == 0)
        {
            if (red_at)
            {
                change_res(P_RED, 0, 5, 1000);

                *(byte*)(GB_HORSES + 1) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
            }
        }

        if (*(byte*)(GB_HORSES + 2) == 0)
        {
            if (green_at)
            {
                change_res(P_GREEN, 0, 5, 1000);

                *(byte*)(GB_HORSES + 2) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
            }
        }

        if (*(byte*)(GB_HORSES + 3) == 0)
        {
            if (violet_at)
            {
                change_res(P_VIOLET, 0, 5, 1000);

                *(byte*)(GB_HORSES + 3) = 1;

                find_all_alive_units(U_DANATH);
                if (units != 0)
                {
                    WORD xx = *((WORD*)((uintptr_t)unit[0] + S_DRAW_X));
                    WORD yy = *((WORD*)((uintptr_t)unit[0] + S_DRAW_Y));
                    ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_BLOOD);
                }
            }
        }
    }
}

void v_xhuman3(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        if ((get_val(ALTAR, local) >= 6))
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }
    }
}

void v_xhuman4(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        find_all_alive_units(U_DANATH);
        if (units == 0)lose(true);
        set_region(3, 99, 4, 100);
        sort_in_region();
        if (units >= 1)
        {
            *(byte*)LEVEL_OBJ = LVL_XHUMAN11;
            *(WORD*)LEVEL_ID = 0x53DA;

            find_all_alive_units(U_DANATH);
            if (units != 0)hero_xp = *((byte*)((uintptr_t)unit[0] + S_KILLS));
            win(true);
        }
    }
}

void v_xhuman5(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman6(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman7(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman8(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman9(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman10(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman11(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xhuman12(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        if (hero_xp != 0)
        {
            find_all_alive_units(U_DANATH);
            if (units != 0)set_stat(unit[0], hero_xp, S_KILLS);
            hero_xp = 0;
        }

        // your custom victory conditions
                // Союзники
        comps_vision(true); // компы могут давать виз
        ally(P_ORANGE, P_BLUE, 1);
        viz(P_ORANGE, P_BLUE, 1);
        viz(P_BLUE, P_ORANGE, 1);

        // Делаем Даната Оранжевым
        find_all_alive_units(U_DANATH);
        set_stat_all(S_COLOR, P_ORANGE);

        // Делаем союзников Оранжевыми
        find_all_alive_units(U_RANGER);
        set_stat_all(S_COLOR, P_ORANGE);

        find_all_alive_units(U_PALADIN);
        set_stat_all(S_COLOR, P_ORANGE);

        // Подкрепление
        if (*(byte*)(GB_HORSES + 14) < 40)
        {
            if (*(byte*)(GB_HORSES + 15) < 200) // таймер на 200
                *(byte*)(GB_HORSES + 15) = *(byte*)(GB_HORSES + 15) + 1;
            else
            {
                // создание подкрепления
                unit_create(127, 122, U_PALADIN, P_ORANGE, 3);
                unit_create(127, 123, U_RANGER, P_ORANGE, 3);
                sound_play_from_file(1, (DWORD)resc_name, resc_sound);
                show_message_from_tbl(tbl_reinforcements, 0);

                *(byte*)(GB_HORSES + 14) += 1; // таймер 1 раз

                *(byte*)(GB_HORSES + 15) = 0; // обнуляем таймер

                // подкрепление прибывает
                find_all_alive_units(U_RANGER);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                order_all(95, 109, ORDER_MOVE);
                give_all(P_BLUE);

                find_all_alive_units(U_PALADIN);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                order_all(95, 109, ORDER_MOVE);
                give_all(P_BLUE);
            }
        }

        // your custom victory conditions
        byte local = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(local))
            lose(true);
        if ((get_val(ALTAR, local) >= 5))
        {
            *(byte*)LEVEL_OBJ = LVL_HUMAN14;
            *(WORD*)LEVEL_ID = 0x52E2;
            win(true);
        }

        if (*(byte*)(GB_HORSES + 8) == 37)win(true);
    }
}

void v_orc1(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        ai_fix_plugin(true);
        saveload_fixed = true;
        // your initialize
    }
    else
    {
        find_all_alive_units(U_EYE);
        if (units == 0)lose(true);

        find_all_alive_units(ANY_UNITS);
        sort_stat(S_OWNER, P_YELLOW, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);

        find_all_alive_units(ANY_UNITS);
        sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);

        find_all_alive_units(ANY_UNITS);
        sort_stat(S_OWNER, P_VIOLET, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);

        find_all_alive_units(ANY_UNITS);
        sort_stat(S_OWNER, P_GREEN, CMP_EQ);
        set_stat_all(S_COLOR, P_WHITE);
        // your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))
            lose(true);
        else
        {
            if (!check_opponents(l))
            {
                *(byte*)LEVEL_OBJ = LVL_XHUMAN12;
                *(WORD*)LEVEL_ID = 0x53DC;
                *(byte*)(GB_HORSES + 8) = 37;
                win(true);
            }
        }
    }
}

void v_orc2(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc3(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc4(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc5(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc6(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc7(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc8(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc9(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc10(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc11(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc12(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc13(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_orc14(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc1(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc2(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc3(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc4(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc5(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc6(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc7(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc8(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc9(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc10(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc11(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_xorc12(bool rep_init)
{
    if (rep_init)
    {
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}

void v_custom(bool rep_init)
{
    if (rep_init)
    {
        // pathfind_fix(true);
        // ai_fix_plugin(true);
        // your initialize
    }
    else
    {
        // your custom victory conditions
    }
}
//-------------------------------------------------------------------------------

void (*triggers[])(bool) = {v_human1, v_orc1, v_human2, v_orc2, v_human3, v_orc3, v_human4, v_orc4, v_human5, v_orc5, v_human6, v_orc6, v_human7, v_orc7, v_human8, v_orc8, v_human9, v_orc9, v_human10, v_orc10, v_human11, v_orc11, v_human12, v_orc12, v_human13, v_orc13, v_human14, v_orc14, v_xhuman1, v_xorc1, v_xhuman2, v_xorc2, v_xhuman3, v_xorc3, v_xhuman4, v_xorc4, v_xhuman5, v_xorc5, v_xhuman6, v_xorc6, v_xhuman7, v_xorc7, v_xhuman8, v_xorc8, v_xhuman9, v_xorc9, v_xhuman10, v_xorc10, v_xhuman11, v_xorc11, v_xhuman12, v_xorc12};

byte regen_timer = 0;

byte remember_lvl = 255;
void trig()
{
    vizs_n = 0; // no reveal map areas every time

    byte lvl = *(byte *)LEVEL_OBJ;
    remember_lvl = lvl;
    if (a_custom)
    {
        v_custom(false);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(false);
        else
            v_custom(false);
    }
    first_step = false;

    // global triggers
    m_devotion[0] = U_DANATH;
    brclik(true);
    A_rally = true;
    repair_cat(true);
    autoheal(true);

    if (regen_timer > 0)regen_timer--;
    else
    {
        if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_UNHOLY)) != 0)
        {
            find_all_alive_units(U_PEASANT);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            heal_all(1, 0);
            find_all_alive_units(U_FOOTMAN);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            heal_all(1, 0);
            find_all_alive_units(U_KNIGHT);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            heal_all(1, 0);
            find_all_alive_units(U_DWARWES);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            heal_all(1, 0);
            find_all_alive_units(U_DANATH);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            heal_all(1, 0);
        }
        regen_timer = 5;
    }

    trigger_time(40);

    autosave_counter++;
    if (autosave_counter >= 75)
    {
        autosave_counter = 0;
        sfile_write_auto();
    }
}

void trig_init()
{
    first_step = true;
    byte lvl = *(byte *)LEVEL_OBJ;
    if (a_custom)
    {
        v_custom(true);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(true);
        else
            v_custom(true);
    }
}

void replace_def()
{
    // set all vars to default
    memset(ua, 255, sizeof(ua));
    memset(ut, 255, sizeof(ut));
    memset(m_devotion, 255, sizeof(m_devotion));
    memset(vizs_areas, 0, sizeof(vizs_areas));
    vizs_n = 0;
    ai_fixed = false;
    saveload_fixed = false;
    A_portal = false;
    A_autoheal = false;
    A_rally = false;
    A_tower = false;
}

void replace_common()
{
    // peon can build any buildings
    char ballbuildings[] = "\x0\x0"; // d1 05
    PATCH_SET((char *)BUILD_ALL_BUILDINGS1, ballbuildings);
    char ballbuildings2[] = "\x0"; // 0a
    PATCH_SET((char *)BUILD_ALL_BUILDINGS2, ballbuildings2);
    char ballbuildings3[] = "\x0"; // 68
    PATCH_SET((char *)BUILD_ALL_BUILDINGS3, ballbuildings3);

    // any building can train any unit
    char ballunits[] = "\xeb"; // 0x74
    PATCH_SET((char *)BUILD_ALL_UNITS1, ballunits);
    char ballunits2[] = "\xA1\xBC\x47\x49\x0\x90\x90"; // 8b 04 85 bc 47 49 00
    PATCH_SET((char *)BUILD_ALL_UNITS2, ballunits2);

    // any building can make any research
    char allres[] = "\xB1\x1\x90";
    PATCH_SET((char *)BUILD_ALL_RESEARCH1, allres);
    PATCH_SET((char *)BUILD_ALL_RESEARCH2, allres);

    // allow all units cast all spells
    char allsp[] = "\x90\x90";
    PATCH_SET((char *)CAST_ALL_SPELLS, allsp);

    // show kills
    byte d = S_KILLS;
    char sdmg[] = "\x8a\x90\x82\x0\x0\x0\x8b\xfa"; // units
    sdmg[2] = d;
    PATCH_SET((char *)SPEED_STAT_UNITS, sdmg);
    char sdmg2[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90"; // catas
    sdmg2[2] = d;
    PATCH_SET((char *)SPEED_STAT_CATS, sdmg2);
    char sdmg3[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90"; // archers
    sdmg3[2] = d;
    PATCH_SET((char *)SPEED_STAT_ARCHERS, sdmg3);
    char sdmg4[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90"; // berserkers
    sdmg4[2] = d;
    PATCH_SET((char *)SPEED_STAT_BERSERKERS, sdmg4);
    char sdmg5[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90"; // ships
    sdmg5[2] = d;
    PATCH_SET((char *)SPEED_STAT_SHIPS, sdmg5);

    char dmg_fix[] = "\xeb";
    PATCH_SET((char *)DMG_FIX, dmg_fix);
}

void replace_back()
{
    // replace all to default
    comps_vision(false);
    repair_cat(false);
    trigger_time('\xc8');
    upgr(SWORDS, 2);
    upgr(ARMOR, 2);
    upgr(ARROWS, 1);
    upgr(SHIP_DMG, 5);
    upgr(SHIP_ARMOR, 5);
    upgr(CATA_DMG, 15);
    manacost(VISION, 70);
    manacost(HEAL, 6);
    manacost(GREATER_HEAL, 5);
    manacost(EXORCISM, 4);
    manacost(FIREBALL, 100);
    manacost(FLAME_SHIELD, 80);
    manacost(SLOW, 50);
    manacost(INVIS, 200);
    manacost(POLYMORPH, 200);
    manacost(BLIZZARD, 25);
    manacost(EYE_OF_KILROG, 70);
    manacost(BLOOD, 50);
    manacost(RAISE_DEAD, 50);
    manacost(COIL, 100);
    manacost(WHIRLWIND, 100);
    manacost(HASTE, 50);
    manacost(UNHOLY_ARMOR, 100);
    manacost(RUNES, 200);
    manacost(DEATH_AND_DECAY, 25);
    ai_fix_plugin(false);
    autoheal(false);
}

void replace_trigger()
{
    replace_back();
    replace_def();
    replace_common();

    // replace original victory trigger
    char trig_jmp[] = "\x74\x1A"; // 74 0F
    PATCH_SET((char *)VICTORY_JMP, trig_jmp);
    char rep[] = "\xc7\x05\x38\x0d\x4c\x0\x30\x8C\x45\x0";
    void (*repf)() = trig;
    patch_setdword((DWORD *)(rep + 6), (DWORD)repf);
    PATCH_SET((char *)VICTORY_TRIGGER, rep);
    trig_init();
}

byte tileset_fog_color = 239;

void tilesets_change_fog(byte color)
{
    if (color == 0)
        color = 239;

    // border between fog and visible tiles
    int adr = (int)(*(int *)TILESET_POINTER_VR4);
    if (adr)
        for (int i = 0; i < 0x4000; i++)
            if (*(byte *)(adr + i) != 0)
                *(byte *)(adr + i) = color;

    // border between fog of war and full fog
    char buf[] = "\x0";
    buf[0] = color;
    for (int i = 0; i < 16; i++)
        PATCH_SET((char *)(0x004019E1 + 4 * i), buf);
    for (int i = 0; i < 16; i++)
        PATCH_SET((char *)(0x00401AC9 + 4 * i), buf);

    // fog of war and full fog and minimap
    tileset_fog_color = color;
}

PROC g_proc_00421E6F;
void tileset_draw_full_fog(int adr)
{
    ((void (*)(int))g_proc_00421E6F)(adr); // original
    int k = 0;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            *(byte *)(adr + k + j) = tileset_fog_color;
        }
        k += m_screen_w;
    }
}

PROC g_proc_00421E81;
void tileset_draw_fog_of_war(int adr)
{
    ((void (*)(int))g_proc_00421E81)(adr); // original
    int k = 0;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j += 2)
        {
            *(byte *)(adr + k + j + ((i % 2 != 0) ? 1 : 0)) = tileset_fog_color;
        }
        k += m_screen_w;
    }
}

PROC g_proc_00421242;
void tileset_draw_fog_minimap()
{
    *(byte *)0x004D6532 = tileset_fog_color;
    ((void (*)())g_proc_00421242)(); // original
    *(byte *)0x004D6532 = 0;
}

void change_controler_stat()
{
    if (remember_lvl == LVL_ORC1)
    {
        *(byte *)(CONTROLER_TYPE + P_BLACK) = C_NOBODY;
        *(byte *)(STARTING_CONTROLER_TYPE + P_BLACK) = C_NOBODY;
    }
    else if ((remember_lvl == LVL_HUMAN1) || (remember_lvl == LVL_XHUMAN1))
    {
        *(byte *)(CONTROLER_TYPE + P_ORANGE) = C_NOBODY;
        *(byte *)(STARTING_CONTROLER_TYPE + P_ORANGE) = C_NOBODY;
    }
    else if ((remember_lvl == LVL_HUMAN10) || (remember_lvl == LVL_HUMAN13) || (remember_lvl == LVL_HUMAN14))
    {
        *(byte *)(CONTROLER_TYPE + P_VIOLET) = C_NOBODY;
        *(byte *)(STARTING_CONTROLER_TYPE + P_VIOLET) = C_NOBODY;
    }
}

PROC g_proc_0042049F;
void game_end_dialog(int a)
{
    //change_controler_stat();
    ((void (*)(int))g_proc_0042049F)(a); // original
}

void set_rain(WORD amount, byte density, byte speed, byte size, byte sz_x, byte sz_y, bool snow, WORD thunder)
{
    can_rain = true;
    raindrops_amount = my_min(RAINDROPS, amount);
    raindrops_density = density;
    raindrops_speed = speed;
    raindrops_size = size;
    raindrops_size_x = my_max(raindrops_size, sz_x);
    raindrops_size_y = my_max(raindrops_size, sz_y);
    raindrops_align_x = raindrops_size - raindrops_size_x;
    raindrops_align_y = raindrops_size - raindrops_size_y;
    raindrops_snow = snow;
    raindrops_thunder = thunder;
    raindrops_thunder_timer = thunder;
    raindrops_thunder_gradient = THUNDER_GRADIENT;
}

byte* ScreenPTR = NULL;
byte* getScreenPtr() {
    DWORD* r;
    r = (DWORD*)SCREEN_POINTER;
    if (r) { return (byte*)*r; }
    return NULL;
}

void draw_pixel(int x, int y, byte c, bool inval)
{
    if (ScreenPTR)
    {
        *(byte*)(ScreenPTR + x + m_screen_w * y) = c;
        if (inval)
            ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x, y);
    }
}

void draw_pixel_safe(int x, int y, byte c, bool inval)
{
    if (ScreenPTR)
    {
        if ((x >= m_minx) && (y >= m_miny) && (x <= m_maxx) && (y <= m_maxy))
        {
            *(byte*)(ScreenPTR + x + m_screen_w * y) = c;
            if (inval)
                ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x, y);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, byte c, bool inval)
{
    int x, y, xdelta, ydelta, xdiff, ydiff, accum, sign;

    xdelta = x2 - x1;
    ydelta = y2 - y1;
    accum = 0;
    sign = 0;

    if (!xdelta && ydelta)
    { /* Special case: straight vertical line */
        x = x1;
        for (y = y1; y < (y1 + ydelta); ++y)
        {
            draw_pixel(x, y, c, true);
        }
    }
    else if (xdelta && !ydelta)
    { /* Special case: straight horisontal line */
        y = y1;
        for (x = x1; x <= x1 + xdelta; ++x)
        {
            draw_pixel(x, y, c, true);
        }
    }
    else
    {
        xdiff = (xdelta << 16) / ydelta;
        ydiff = (ydelta << 16) / xdelta;

        if (abs(xdiff) > abs(ydiff)) { /* horizontal-major */
            y = y1;
            if (xdelta < 0) { /* traversing negative x */
                for (x = x1; x >= x2; --x) {
                    draw_pixel(x, y, c, true);
                    accum += abs(ydiff);
                    while (accum >= (1 << 16)) {
                        ++y;
                        accum -= (1 << 16);
                    }
                }
            }
            else { /* traversing positive x */
                for (x = x1; x <= x2; ++x) {
                    draw_pixel(x, y, c, true);
                    accum += abs(ydiff);
                    while (accum >= (1 << 16)) {
                        ++y;
                        accum -= (1 << 16);
                    }
                }
            }
        }
        else if (abs(ydiff) > abs(xdiff)) { /* vertical major */
            sign = (xdelta > 0 ? 1 : -1);
            x = x1;
            for (y = y1; y <= y2; ++y) {
                draw_pixel(x, y, c, true);
                accum += abs(xdiff);
                while (accum >= (1 << 16)) {
                    x += sign;
                    accum -= (1 << 16);
                }
            }
        }
        else if (abs(ydiff) == abs(xdiff)) { /* 45 degrees */
            sign = (xdelta > 0 ? 1 : -1);
            x = x1;
            for (y = y1; y <= y2; ++y) {
                draw_pixel(x, y, c, true);
                x += sign;
            }
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

void draw_rain()
{
    //set_rain_color(raindrops_color_r, raindrops_color_g, raindrops_color_b);
    for (int i = 0; i < raindrops_amount; i++)
    {
        if (raindrops[i].l != 0)
        {
            if ((raindrops[i].x1 >= m_minx) && (raindrops[i].y1 >= m_miny) && (raindrops[i].x2 <= m_maxx) && (raindrops[i].y2 <= m_maxy))
            {
                if (raindrops_snow)
                {
                    WORD cx = raindrops[i].x1 + (abs(raindrops[i].x2 - raindrops[i].x1) / 2);
                    WORD cy = raindrops[i].y1 + (abs(raindrops[i].y2 - raindrops[i].y1) / 2);
                    draw_pixel_safe(cx - 1, cy, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 2, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 2, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 2, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 2, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 1, cy - 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy - 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 1, cy + 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy + 2, RAINDROPS_COLOR, false);
                }
                else draw_line(raindrops[i].x1, raindrops[i].y1, raindrops[i].x2, raindrops[i].y2, RAINDROPS_COLOR, false);
            }
        }
    }
    ((void (*)(int, int, int, int))F_INVALIDATE)(m_minx, m_miny, m_maxx, m_maxy);
}

void draw_pixel_noinval(int x, int y, byte c)
{
    if (ScreenPTR)*(byte*)(ScreenPTR + x + m_screen_w * y) = c;
}

void draw_pixel_noinval_safe(int x, int y, byte c)
{
    if (x < m_minx)return;
    if (x > m_maxx)return;
    if (y < m_miny)return;
    if (y > m_maxy)return;

    if (ScreenPTR)*(byte*)(ScreenPTR + x + m_screen_w * y) = c;
}

void draw_line_fast_break_safe(int x1, int y1, int x2, int y2, byte c, bool inval, int brk, int sbrk)
{
    int br = sbrk;

    bool yLonger = false;
    int incrementVal;
    int shortLen = y2 - y1;
    int longLen = x2 - x1;

    if (abs(shortLen) > abs(longLen)) {
        int swap = shortLen;
        shortLen = longLen;
        longLen = swap;
        yLonger = true;
    }

    if (longLen < 0) incrementVal = -1;
    else incrementVal = 1;

    double multDiff;
    if (longLen == 0.0) multDiff = (double)shortLen;
    else multDiff = (double)shortLen / (double)longLen;
    if (yLonger) {
        for (int i = 0; i != longLen; i += incrementVal) {
            if (br < brk)draw_pixel_noinval_safe(x1 + (int)((double)i * multDiff), y1 + i, c);
            if (br > (2 * brk))br = 0;
            br++;
        }
    }
    else {
        for (int i = 0; i != longLen; i += incrementVal) {
            if (br < brk)draw_pixel_noinval_safe(x1 + i, y1 + (int)((double)i * multDiff), c);
            if (br > (2 * brk))br = 0;
            br++;
        }
    }

    int tmp = 0;
    if (x2 < x1)
    {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y2 < y1)
    {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

void draw_rect(int x1, int y1, int x2, int y2, byte c, bool o, bool inval)
{
    if (!ScreenPTR)return;
    int xmin = my_min(x1, x2);
    int xmax = my_max(x1, x2);
    int ymin = my_min(y1, y2);
    int ymax = my_max(y1, y2);
    byte* p = ScreenPTR + ymin * m_screen_w + xmin;
    if (o)
    {
        for (int i = xmax - xmin + 1; i > 0; i--)
        {
            *p++ = c;
        }
        p = ScreenPTR + ymin * m_screen_w + xmin;
        for (int i = ymax - ymin + 1; i > 0; i--)
        {
            *p = c;
            p += m_screen_w;
        }
        p = ScreenPTR + ymax * m_screen_w + xmin;
        for (int i = xmax - xmin + 1; i > 0; i--)
        {
            *p++ = c;
        }
        p = ScreenPTR + ymin * m_screen_w + xmax;
        for (int i = ymax - ymin + 1; i > 0; i--)
        {
            *p = c;
            p += m_screen_w;
        }
    }
    else
    {
        for (int j = ymax - ymin + 1; j > 0; j--)
        {
            for (int i = xmax - xmin + 1; i > 0; i--)
                *p++ = c;
            p += m_screen_w - xmax + xmin - 1;
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

void draw_circle_safe(int x, int y, int r, byte c, bool o, bool inval)
{
    int x1 = x - r;
    int x2 = x + r;
    int y1 = y - r;
    int y2 = y + r;
    if (x1 < 0)x1 = 0;
    if (x2 >= m_screen_w)x2 = m_screen_w;
    if (y1 < 0)y1 = 0;
    if (y2 >= m_screen_h)y2 = m_screen_h;

    if (x1 < m_minx)return;
    if (x2 > m_maxx)return;
    if (y1 < m_miny)return;
    if (y2 > m_maxy)return;

    for (int i = x1; i < x2; i++)
    {
        for (int j = y1; j < y2; j++)
        {
            double d = sqrt((double)((x - i) * (x - i) + (y - j) * (y - j)));
            if (o)
            {
                if (ceil(d) == r)draw_pixel_noinval(i, j, c);
            }
            else
            {
                if (d < r)draw_pixel_noinval(i, j, c);
            }
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

void draw_sprite(int x, int y, byte* s, int w, int h, bool a, bool inval)
{
    if (!ScreenPTR)return;
    if (s != NULL)
    {
        byte* p = ScreenPTR + y * m_screen_w + x;
        byte* ps = s;
        if (!a)
            for (int j = h - 1; j >= 0; j--)
            {
                for (int i = w - 1; i >= 0; i--)
                    *p++ = *ps++;
                p += m_screen_w - w;
            }
        else
            for (int j = h - 1; j >= 0; j--)
            {
                for (int i = w - 1; i >= 0; i--)
                {
                    if (*ps) *p = *ps;
                    p++;
                    ps++;
                }
                p += m_screen_w - w;
            }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + h);
}

void draw_sprite_safe(int x, int y, byte* s, int w, int h, bool a, bool inval)
{
    if (x < m_minx)return;
    if ((x + w) > m_maxx)return;
    if (y < m_miny)return;
    if ((y + h) > m_maxy)return;

    if (!ScreenPTR)return;
    if (s != NULL)
    {
        byte* p = ScreenPTR + y * m_screen_w + x;
        byte* ps = s;
        if (!a)
            for (int j = h - 1; j >= 0; j--)
            {
                for (int i = w - 1; i >= 0; i--)
                    *p++ = *ps++;
                p += m_screen_w - w;
            }
        else
            for (int j = h - 1; j >= 0; j--)
            {
                for (int i = w - 1; i >= 0; i--)
                {
                    if (*ps) *p = *ps;
                    p++;
                    ps++;
                }
                p += m_screen_w - w;
            }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + h);
}

void draw_sprite_400slower_safe_screen(int x, int y, byte* s, int w, int h, bool a, bool inval)
{
    if (s != NULL)
    {
        for (int i = x; i <= x + w - 1; i++)
        {
            for (int j = y; j <= y + h - 1; j++)
            {
                if (i < 0)continue;
                if (j < 0)continue;
                if (i >= m_screen_w)continue;
                if (j >= m_screen_h)continue;

                if (!a)
                    draw_pixel(i, j, s[(j - y) * w + (i - x)], false);
                else
                {
                    if (s[(j - y) * w + (i - x)] != 0)
                        draw_pixel(i, j, s[(j - y) * w + (i - x)], false);
                }
            }
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + h);
}

int draw_char(int x, int y, byte c, unsigned char ch, byte bc, bool inval)
{
    if (ch < ' ')ch = ' ';
    ch -= ' ';
    int chMfontwidthplus1 = ch * (1 + FONT_6PX_PROP_CHAR_WIDTH);
    int w = 1 + font_6px_prop[chMfontwidthplus1];

    //if (x < m_minx)return 0;
    //if ((x + w) >= m_maxx)return 0;
    //if (y < m_miny)return 0;
    //if ((y + 8) > m_maxy)return 0;

    if (!ScreenPTR)return 0;
    byte* p = ScreenPTR + y * m_screen_w + x;

    if (!((c == 0) && (bc == 0)))
    {
        if (bc)
            for (int i = 1; i <= w; i++)
            {
                byte font_line = font_6px_prop[chMfontwidthplus1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    else *p = bc;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
        else
            for (int i = 0; i < w; i++)
            {
                byte font_line = font_6px_prop[ch * (1 + FONT_6PX_PROP_CHAR_WIDTH) + 1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

int draw_text(int x, int y, byte c, unsigned char* ch, byte bc, byte cond, bool inval)
{
    int w = 0;
    int C = 0;
    while (ch[C] != 0)
    {
        w += draw_char(x + w, y, c, ch[C], bc, false);
        w -= cond;
        if (w < 0)w = 0;
        C++;
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

int draw_char_get_width(unsigned char ch)
{
    if (ch < ' ')ch = ' ';
    return 1 + font_6px_prop[(ch - ' ') * (1 + FONT_6PX_PROP_CHAR_WIDTH)];
}

int draw_text_get_width(unsigned char* ch, int s, byte cond)
{
    int w = 0;
    int C = 0;
    while (ch[C] != 0)
    {
        w += draw_char_get_width(ch[C]);
        w -= cond;
        if (w < 0)w = 0;
        C++;
    }
    return w * s;
}

int draw_char_safe(int x, int y, byte c, unsigned char ch, byte bc, bool inval)
{
    if (ch < ' ')ch = ' ';
    ch -= ' ';
    int chMfontwidthplus1 = ch * (1 + FONT_6PX_PROP_CHAR_WIDTH);
    int w = 1 + font_6px_prop[chMfontwidthplus1];

    if (x < m_minx)return 0;
    if ((x + w) >= m_maxx)return 0;
    if (y < m_miny)return 0;
    if ((y + 8) > m_maxy)return 0;

    if (!ScreenPTR)return 0;
    byte* p = ScreenPTR + y * m_screen_w + x;

    if (!((c == 0) && (bc == 0)))
    {
        if (bc)
            for (int i = 1; i <= w; i++)
            {
                byte font_line = font_6px_prop[chMfontwidthplus1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    else *p = bc;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
        else
            for (int i = 0; i < w; i++)
            {
                byte font_line = font_6px_prop[ch * (1 + FONT_6PX_PROP_CHAR_WIDTH) + 1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

int draw_text_safe(int x, int y, byte c, unsigned char* ch, byte bc, byte cond, bool inval)
{
    int w = 0;
    int C = 0;
    while (ch[C] != 0)
    {
        w += draw_char_safe(x + w, y, c, ch[C], bc, false);
        w -= cond;
        if (w < 0)w = 0;
        C++;
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

#define _bmp_current *((void**)0x004D5008)
#define img_draw_buf ((void*)0x004D4A90)
#define gpMedFont   *((void**)0x004D4FE0)

void (*font_puts)(int x, int y, char* pszStr) = (void(*)(int, int, char*))0x00486E60;
void* (*font_get)(void) = (void* (*)(void))0x00486BC0;
void (*font_set)(void* pFont) = (void(*)(void*))0x00486BD0;

void draw_text_original(int x, int y, char* txt)
{
    void* old_bmp = _bmp_current;
    _bmp_current = img_draw_buf;
    void* old_font = font_get();
    font_set(gpMedFont);
    font_puts(x, y, txt);
    font_set(old_font);
    _bmp_current = old_bmp;
}

#define GAME_MODE 0x004AE430

WORD get_default_frame(byte id, byte o)
{
    //train unit = 0, research spell = 1, research tech = 2, upgrade building = 3
    WORD units_icons[] = { 2,3,0,1,16,17,8,9,4,5,14,15,10,11,12,13,0,1,6,7,187,189,191,194,193,190,18,19,20,21,22,23,24,25,0,192,0,0,26,27,28,29,30,31,195,111,188,186,0,36,32,33,34,35,0,114,37,115 };
    WORD tech_icons[] = { 117,118,120,121,125,126,128,129,165,166,168,169,145,146,148,149,154,155,151,152,138,139,140,141,6,132,133,134,7,135,136,137,11,10,106,107,110,100,101,94,95,115,105,111,112,114,108,104,96,98,97,103 };
    if (o == 0)
    {
        if (id < U_FARM)return units_icons[id];
    }
    else if (o == 1)
    {
        if (id <= UG_SP_ROT)return tech_icons[id];
    }
    else if (o == 2)
    {
        if (id <= UG_SP_ROT)return tech_icons[id];
    }
    else if (o == 3)
    {
        if ((id >= U_KEEP) && (id <= U_FORTRESS))return 66 + (id - U_KEEP);
        if ((id >= U_HARROWTOWER) && (id <= U_OCANONTOWER))
        {
            if (id == U_HARROWTOWER)return 75;
            if (id == U_HCANONTOWER)return 76;
            if (id == U_OARROWTOWER)return 77;
            if (id == U_OCANONTOWER)return 78;
        }
    }
    return 0xFFFF;
}

#define RANK1 2
#define RANK2 5
#define RANK3 10
#define RANK4 15
#define RANK5 25

byte get_next_rank(byte xp)
{
    if (xp < RANK1)return RANK1;
    else if (xp < RANK2)return RANK2;
    else if (xp < RANK3)return RANK3;
    else if (xp < RANK4)return RANK4;
    else if (xp < RANK5)return RANK5;
    return 255;
}

byte get_prev_rank(byte xp)
{
    if (xp >= RANK5)return RANK5;
    else if (xp >= RANK4)return RANK4;
    else if (xp >= RANK3)return RANK3;
    else if (xp >= RANK2)return RANK2;
    else if (xp >= RANK1)return RANK1;
    return 0;
}

byte get_rank_frame(byte nk)
{
    byte f = 0;
    if (nk == RANK1)f = 0;
    else if (nk == RANK2)f = 2;
    else if (nk == RANK3)f = 4;
    else if (nk == RANK4)f = 6;
    else if (nk == RANK5)f = 8;
    else if (nk > RANK5)f = 10;
    return f;
}

bool draw_status = false;

void drawing_status()
{
    if (draw_status)
    {
        ScreenPTR = getScreenPtr();
        if ((*(byte*)GAME_MODE == 3) && (game_started))
        {
            int* p = (int*)*(int*)0x004BDC78;//gpStatUnit
            if (p)
            {
                char text[MAX_PATH];
                memset(text, 0, sizeof(text));

                byte ow = *((byte*)((uintptr_t)p + S_OWNER));
                byte id = *((byte*)((uintptr_t)p + S_ID));

                WORD dmgl = 0;
                WORD dmgh = 0;
                WORD dmgs = *(byte*)(UNIT_STRENGTH_TABLE + id);
                WORD dmgp = *(byte*)(UNIT_PIERCE_TABLE + id);
                WORD dmgsb = 0;
                WORD dmgpb = 0;
                WORD armor = *(byte*)(UNIT_ARMOR_TABLE + id);
                WORD armorb = 0;
                DWORD vzf = *(DWORD*)(UNIT_VISION_FUNCTIONS_TABLE + 4 * id);
                WORD vz = 0;
                switch (vzf)
                {
                case F_VISION1:vz = 1; break;
                case F_VISION2:vz = 2; break;
                case F_VISION3:vz = 3; break;
                case F_VISION4:vz = 4; break;
                case F_VISION5:vz = 5; break;
                case F_VISION6:vz = 6; break;
                case F_VISION7:vz = 7; break;
                case F_VISION8:vz = 8; break;
                case F_VISION9:vz = 9; break;
                default:vz = 1; break;
                }
                WORD rng = *(byte*)(UNIT_RANGE_TABLE + id);
                WORD rngb = 0;
                WORD spd = *(byte*)(0x004BB824 + id);//unit speed table
                double spdb = 0;
                WORD aspd = (WORD)round(100 * ((double)*(byte*)(0x004BB89C + U_FOOTMAN) / (double)*(byte*)(0x004BB89C + id)));//004BB89C  gbUnitAttackDelay
                double aspdb = 0;
                WORD lvl = 1;

                byte wu = *(byte*)(0x004CE9D0 + id);//have weapon upgr table
                byte au = *(byte*)(0x004CEBCC + id);//have armor upgr table
                byte mag = (byte)((*(int*)(UNIT_GLOBAL_FLAGS + id * 4) & IS_CASTER) != 0);
                byte ca = (byte)((*(int*)(UNIT_GLOBAL_FLAGS + id * 4) & IS_ATTACKER) != 0);
                byte lf = *(byte*)(0x004CFBC4 + id);//life time table

                if (id >= U_FARM)
                {
                    byte a = 0;

                    if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER))
                    {
                        if (wu != 0)
                        {
                            a = *(byte*)(GB_SWORDS + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                dmgsb += a * (*(byte*)(UPGRD + SWORDS));
                            }
                        }
                    }

                    dmgl = dmgs > 30 ? ((dmgs - 30) / 2) : 0;
                    dmgl += (dmgp + 1) / 2;
                    dmgh = dmgs + dmgp;
                    if (dmgl > 255)dmgl = 255;
                    if (dmgh > 255)dmgh = 255;

                    if (au != 0)
                    {
                        a = *(byte*)(GB_SHIELDS + ow);
                        if (a > 0)
                        {
                            lvl += a;
                            armorb += a * (*(byte*)(UPGRD + ARMOR));
                        }
                    }

                    if (!((wu == 0) && (au == 0)))
                    {
                        sprintf(text, "%s %d", text_from_tbl(tbl_stats, 0), lvl);
                        draw_text_original(80, 200, text);
                    }

                    WORD goldb = 0;
                    WORD lumb = 0;
                    WORD oilb = 0;

                    if (get_val(TH2, ow) != 0)goldb = 10;
                    if (get_val(TH3, ow) != 0)goldb = 20;
                    if (get_val(LUMBERMILL, ow) != 0)lumb = 25;
                    if (get_val(REFINERY, ow) != 0)oilb = 25;
                    
                    //custom bonuses calc
                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_DD)) != 0)lumb += 25;
                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_HASTE)) != 0)goldb += 10;
                    find_all_alive_units(U_CHURCH);
                    sort_stat(S_OWNER, ow, CMP_EQ);
                    sort_complete();
                    int market = units;
                    lumb += 5 * market;
                    goldb += 5 * market;

                    WORD res = *((WORD*)((uintptr_t)p + S_RESOURCES));

                    if (ow == *(byte*)LOCAL_PLAYER)
                    {
                        byte od = *((byte*)((uintptr_t)p + S_ORDER));
                        if (check_unit_complete(p) && (od == ORDER_BLDG_BUILD))
                        {
                            char* bo_text0 = text_from_tbl(tbl_stats, 14);
                            char* bo_text1 = text_from_tbl(tbl_stats, 15);
                            char* bo_text2 = text_from_tbl(tbl_stats, 16);
                            char* bo_text3 = text_from_tbl(tbl_stats, 17);
                            char* bo_text = bo_text3;
                            byte bo = *((byte*)((uintptr_t)p + S_BUILD_ORDER));
                            if (bo == 0)bo_text = bo_text0;
                            else if (bo == 1)bo_text = bo_text1;
                            else if (bo == 2)bo_text = bo_text2;
                            else if (bo == 3)bo_text = bo_text3;
                            sprintf(text, "%s:", bo_text);
                            draw_text_original(13, 254, text);
                        }
                        if (check_unit_complete(p) && (od != ORDER_BLDG_BUILD))
                        {

                            //custom stuck for buildings lvl upgrade
                            if ((id == U_FARM) || (id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_HLUMBER))
                            {
                                if ((id == U_FARM) || (id == U_TOWN_HALL) || (id == U_KEEP))
                                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_HASTE)) != 0)lvl++;
                                if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_HLUMBER))
                                {
                                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_DD)) != 0)lvl++;
                                    find_all_alive_units(U_CHURCH);
                                    sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
                                    sort_complete();
                                    lvl += units;
                                }

                                sprintf(text, "%s %d", text_from_tbl(tbl_stats, 0), lvl);
                                draw_text_original(80, 200, text);
                            }
                            //custom stuck for buildings lvl upgrade

                            if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER))
                            {
                                char* dtext = text_from_tbl(tbl_stats, 1);
                                char* dtext2 = text_from_tbl(tbl_stats, 2);
                                if (ca == 1)
                                {
                                    if (dmgh == dmgl)
                                    {
                                        if ((dmgsb != 0) && (dmgpb != 0))sprintf(text, "%s: %d\x3+%d\x4+%d", dtext, dmgh, dmgpb, dmgsb);
                                        else if (dmgsb != 0)sprintf(text, "%s: %d\x4+%d", dtext, dmgh, dmgsb);
                                        else if (dmgpb != 0)sprintf(text, "%s: %d\x3+%d", dtext, dmgh, dmgpb);
                                        else sprintf(text, "%s: %d", dtext, dmgh);
                                    }
                                    else
                                    {
                                        if ((dmgsb != 0) && (dmgpb != 0))sprintf(text, "%s: %d-%d\x3+%d\x4+%d", dtext, dmgl, dmgh, dmgpb, dmgsb);
                                        else if (dmgsb != 0)sprintf(text, "%s: %d-%d\x4+%d", dtext, dmgl, dmgh, dmgsb);
                                        else if (dmgpb != 0)sprintf(text, "%s: %d-%d\x3+%d", dtext, dmgl, dmgh, dmgpb);
                                        else sprintf(text, "%s: %d-%d", dtext, dmgl, dmgh);
                                    }
                                }
                                else sprintf(text, "%s", dtext2);
                                draw_text_original(12, 228, text);

                                char* atext = text_from_tbl(tbl_stats, 3);
                                if (armorb == 0)sprintf(text, "%s: %d", atext, armor);
                                else sprintf(text, "%s: %d\x4+%d", atext, armor, armorb);
                                draw_text_original(12, 245, text);

                                char* vtext = text_from_tbl(tbl_stats, 4);
                                sprintf(text, "%s: %d", vtext, vz);
                                draw_text_original(12, 262, text);

                                char* rtext = text_from_tbl(tbl_stats, 5);
                                if (rngb == 0)sprintf(text, "%s: %d", rtext, rng);
                                else sprintf(text, "%s: %d\x4+%d", rtext, rng, rngb);
                                draw_text_original(12 + 72, 262, text);

                                if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER))aspd = 42;
                                if ((id == U_HCANONTOWER) || (id == U_OCANONTOWER))aspd = 16;

                                char* astext = text_from_tbl(tbl_stats, 6);
                                if (ca == 0)aspd = 0;
                                sprintf(text, "%s: %d", astext, aspd);
                                draw_text_original(12, 279, text);
                            }
                            else
                            {
                                char* atext = text_from_tbl(tbl_stats, 3);
                                if (armorb == 0)sprintf(text, "%s: %d", atext, armor);
                                else sprintf(text, "%s: %d\x4+%d", atext, armor, armorb);
                                draw_text_original(12, 228, text);

                                char* vtext = text_from_tbl(tbl_stats, 4);
                                sprintf(text, "%s: %d", vtext, vz);
                                draw_text_original(12, 245, text);

                                if ((id == U_FARM) || (id == U_PIGFARM))
                                {
                                    char* ftext = text_from_tbl(tbl_stats, 18);
                                    sprintf(text, "%s: %d/%d", ftext, get_val(ALL_UNITS, ow) - get_val(NPC, ow), get_val(FOOD_LIMIT, ow));
                                    draw_text_original(12, 262, text);

                                    //custom food
                                    int food = 4;
                                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_HASTE)) != 0)food += 1;

                                    char* ftext2 = text_from_tbl(tbl_stats, 19);
                                    sprintf(text, "%s: \x4+%d", ftext2, food);//change if custom food
                                    draw_text_original(12, 279, text);
                                }
                                else if ((id == U_TOWN_HALL) || (id == U_GREAT_HALL) || (id == U_KEEP) || (id == U_STRONGHOLD) || (id == U_CASTLE) || (id == U_FORTRESS))
                                {
                                    char* gtext = text_from_tbl(tbl_stats, 20);
                                    if (goldb == 0)sprintf(text, "%s: %d", gtext, 100);
                                    else sprintf(text, "%s: %d+\x4%d", gtext, 100, goldb);
                                    draw_text_original(12, 262, text);

                                    char* ltext = text_from_tbl(tbl_stats, 21);
                                    if (lumb == 0)sprintf(text, "%s: %d", ltext, 100);
                                    else sprintf(text, "%s: %d+\x4%d", ltext, 100, lumb);
                                    draw_text_original(12, 279, text);

                                    char* otext = text_from_tbl(tbl_stats, 22);
                                    if (oilb == 0)sprintf(text, "%s: %d", otext, 100);
                                    else sprintf(text, "%s: %d+\x4%d", otext, 100, oilb);
                                    draw_text_original(12, 296, text);

                                    char* ftext = text_from_tbl(tbl_stats, 19);
                                    sprintf(text, "%s: \x4+%d", ftext, 1);//change if custom food
                                    draw_text_original(12, 313, text);
                                }
                                else if ((id == U_HLUMBER) || (id == U_OLUMBER))
                                {
                                    char* ltext = text_from_tbl(tbl_stats, 21);
                                    if (lumb == 0)sprintf(text, "%s: %d", ltext, 100);
                                    else sprintf(text, "%s: %d+\x4%d", ltext, 100, lumb);
                                    draw_text_original(12, 262, text);
                                }
                                else if ((id == U_HREFINERY) || (id == U_OREFINERY))
                                {
                                    char* otext = text_from_tbl(tbl_stats, 22);
                                    if (oilb == 0)sprintf(text, "%s: %d", otext, 100);
                                    else sprintf(text, "%s: %d+\x4%d", otext, 100, oilb);
                                    draw_text_original(12, 262, text);
                                }
                                else if ((id == U_HPLATFORM) || (id == U_OPLATFORM))
                                {
                                    char* otext = text_from_tbl(tbl_stats, 22);
                                    sprintf(text, "%s: %d", otext, res * 100);
                                    draw_text_original(60, 264, text);
                                }
                            }
                        }
                    }
                    else
                    {
                        if (id == U_OIL)
                        {
                            //hide draw oil for this campanign
                            //char* otext = text_from_tbl(tbl_stats, 22);
                            //sprintf(text, "%s: %d", otext, res * 100);
                            //draw_text_original(60, 264, text);
                        }
                        else if (id == U_MINE)
                        {
                            char* gtext = text_from_tbl(tbl_stats, 20);
                            sprintf(text, "%s: %d", gtext, res * 100);
                            draw_text_original(60, 264, text);
                        }
                    }
                }
                else if (id < U_FARM)
                {
                    byte a = 0;

                    if ((id == U_ARCHER) || (id == U_TROLL) || (id == U_RANGER) || (id == U_BERSERK))
                    {
                        if (wu != 0)
                        {
                            a = *(byte*)(GB_ARROWS + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                dmgpb += a * (*(byte*)(UPGRD + ARROWS));
                            }
                        }
                        if ((id == U_RANGER) || (id == U_BERSERK))
                        {
                            lvl++;
                            a = *(byte*)(GB_MARKS + ow);
                            if (a)
                            {
                                lvl++;
                                if (id == U_RANGER)
                                {
                                    dmgpb += dmgs;
                                    dmgs = 0;
                                }
                            }
                            a = *(byte*)(GB_LONGBOW + ow);
                            if (a)
                            {
                                lvl++;
                                rngb += 1;
                            }
                            a = *(byte*)(GB_SCOUTING + ow);
                            if (a)
                            {
                                lvl++;
                                vz = 9;
                            }
                        }
                    }
                    else if ((id == U_BALLISTA) || (id == U_CATAPULT))
                    {
                        if (wu != 0)
                        {
                            a = *(byte*)(GB_CAT_DMG + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                dmgsb += a * (*(byte*)(UPGRD + CATA_DMG));
                            }
                        }
                    }
                    else if ((id == U_HTANKER) || (id == U_OTANKER) ||
                        (id == U_HDESTROYER) || (id == U_ODESTROYER) ||
                        (id == U_BATTLESHIP) || (id == U_JUGGERNAUT) ||
                        (id == U_HTRANSPORT) || (id == U_OTRANSPORT) ||
                        (id == U_SUBMARINE) || (id == U_TURTLE))
                    {
                        if (wu != 0)
                        {
                            a = *(byte*)(GB_BOAT_ATTACK + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                dmgsb += a * (*(byte*)(UPGRD + SHIP_DMG));
                            }
                        }
                    }
                    else
                    {
                        if (wu != 0)
                        {
                            a = *(byte*)(GB_SWORDS + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                dmgsb += a * (*(byte*)(UPGRD + SWORDS));
                            }
                        }
                    }

                    dmgl = dmgs > 30 ? ((dmgs - 30) / 2) : 0;
                    dmgl += (dmgp + 1) / 2;
                    dmgh = dmgs + dmgp;
                    if (dmgl > 255)dmgl = 255;
                    if (dmgh > 255)dmgh = 255;

                    if (au != 0)
                    {
                        if ((id == U_HTANKER) || (id == U_OTANKER) ||
                            (id == U_HDESTROYER) || (id == U_ODESTROYER) ||
                            (id == U_BATTLESHIP) || (id == U_JUGGERNAUT) ||
                            (id == U_HTRANSPORT) || (id == U_OTRANSPORT) ||
                            (id == U_SUBMARINE) || (id == U_TURTLE))
                        {
                            a = *(byte*)(GB_BOAT_ARMOR + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                armorb += a * (*(byte*)(UPGRD + SHIP_ARMOR));
                            }
                        }
                        else
                        {
                            a = *(byte*)(GB_SHIELDS + ow);
                            if (a > 0)
                            {
                                lvl += a;
                                armorb += a * (*(byte*)(UPGRD + ARMOR));
                            }
                        }
                    }

                    int spl = *(int*)(SPELLS_LEARNED + 4 * ow);
                    if ((id == U_MAGE) || (id == U_HADGAR))
                    {
                        if (spl & (1 << L_FLAME_SHIELD))lvl++;
                        if (spl & (1 << L_SLOW))lvl++;
                        if (spl & (1 << L_INVIS))lvl++;
                        if (spl & (1 << L_POLYMORF))lvl++;
                        if (spl & (1 << L_BLIZZARD))lvl++;
                    }
                    else if ((id == U_DK) || (id == U_TERON) || (id == U_GULDAN))
                    {
                        if (spl & (1 << L_RAISE))lvl++;
                        if (spl & (1 << L_HASTE))lvl++;
                        if (spl & (1 << L_WIND))lvl++;
                        if (spl & (1 << L_UNHOLY))lvl++;
                        if (spl & (1 << L_DD))lvl++;
                    }
                    else if ((id == U_PALADIN) || (id == U_UTER) || (id == U_TYRALYON))
                    {
                        if (id == U_PALADIN)lvl++;
                        if (spl & (1 << L_HEAL))lvl++;
                        if (spl & (1 << L_EXORCISM))lvl++;
                    }
                    else if ((id == U_OGREMAGE) || (id == U_DENTARG))
                    {
                        if (id == U_OGREMAGE)lvl++;
                        if (spl & (1 << L_BLOOD))lvl++;
                        if (spl & (1 << L_RUNES))lvl++;
                    }

                    //custom upgrades calculation here

                    if (ow == *(byte*)LOCAL_PLAYER)
                    {
                        //peon chop faster upgrade
                        if (id == U_PEASANT)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_RAISE)) != 0)lvl++;
                        }

                        //armored dwarf custom upgrades
                        if (id == U_KNIGHT)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_RUNES)) != 0)
                            {
                                armorb += 4;
                                lvl++;
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_BLIZZARD)) != 0)
                            {
                                dmgpb += 3;
                                lvl++;
                            }
                            spd = 8;
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_FLAME_SHIELD)) != 0)
                            {
                                spdb += 5.3;
                                lvl++;
                            }
                        }

                        //regen upgr
                        if ((*(DWORD*)(SPELLS_LEARNED + 4 * ow) & (1 << L_UNHOLY)) != 0)
                        {
                            if (id == U_PEASANT)lvl++;
                            if (id == U_FOOTMAN)lvl++;
                            if (id == U_KNIGHT)lvl++;
                            if (id == U_DWARWES)lvl++;
                            if (id == U_DANATH)lvl++;
                        }
                    }

                    //custom upgrades calculation here

                    bool draw_ranks = true;

                    if ((id == U_HTRANSPORT) || (id == U_OTRANSPORT))draw_ranks = false;

                    //stuck for draw secret heroes names instead of lvl
                    WORD uid = unit_fixup(p);
                    if (id == U_ZULJIN)
                    {
                        if (names_secret[uid] == 0)names_secret[uid] = rand() % 15;
                        sprintf(text, "%s", text_from_tbl(tbl_names_secret1, names_secret[uid]));
                        draw_text_original(70, 200, text);
                    }
                    else if ((id == U_KARGATH) && (ow != P_NEUTRAL))
                    {
                        if (names_secret[uid] == 0)names_secret[uid] = rand() % 28;
                        sprintf(text, "%s", text_from_tbl(tbl_names_secret2, names_secret[uid]));
                        draw_text_original(70, 200, text);
                    }
                    else
                    {
                        sprintf(text, "%s %d", text_from_tbl(tbl_stats, 0), lvl);
                        draw_text_original(80, 200, text);
                    }

                    //default draw lvl
                    //sprintf(text, "%s %d", text_from_tbl(tbl_stats, 0), lvl);
                    //draw_text_original(80, 200, text);

                    if (ow != *(byte*)LOCAL_PLAYER)
                    {
                        if (draw_ranks)
                        {
                            grp_make_rank(get_rank_frame(get_next_rank(*((byte*)((uintptr_t)p + S_KILLS)))));
                            draw_sprite(96, 214, spr_rank, 28, 14, true, false);
                        }
                    }

                    bool fdraw = true;
                    if ((id == U_HTRANSPORT) || (id == U_OTRANSPORT))
                    {
                        for (int i = 0; i < 6; i++)if (*((WORD*)((uintptr_t)p + S_PEON_GOLDMINE_POINTER + 2 * i)) != 0xFFFF)fdraw = false;
                    }

                    if (fdraw && (ow == *(byte*)LOCAL_PLAYER))
                    {
                        bool pch = false;
                        if ((id == U_PEASANT) || (id == U_PEON))
                        {
                            byte odr = *((byte*)((uintptr_t)p + S_ORDER));
                            if (odr == ORDER_HARVEST)
                            {
                                byte pf = *((byte*)((uintptr_t)p + S_PEON_FLAGS));
                                if ((pf & PEON_HARVEST_LUMBER) != 0)pch = true;
                            }
                        }

                        char* dtext = text_from_tbl(tbl_stats, 1);
                        char* dtext2 = text_from_tbl(tbl_stats, 2);
                        if (ca == 1)
                        {
                            if (dmgh == dmgl)
                            {
                                if ((dmgsb != 0) && (dmgpb != 0))sprintf(text, "%s: %d\x3+%d\x4+%d", dtext, dmgh, dmgpb, dmgsb);
                                else if (dmgsb != 0)sprintf(text, "%s: %d\x4+%d", dtext, dmgh, dmgsb);
                                else if (dmgpb != 0)sprintf(text, "%s: %d\x3+%d", dtext, dmgh, dmgpb);
                                else sprintf(text, "%s: %d", dtext, dmgh);
                            }
                            else
                            {
                                if ((dmgsb != 0) && (dmgpb != 0))sprintf(text, "%s: %d-%d\x3+%d\x4+%d", dtext, dmgl, dmgh, dmgpb, dmgsb);
                                else if (dmgsb != 0)sprintf(text, "%s: %d-%d\x4+%d", dtext, dmgl, dmgh, dmgsb);
                                else if (dmgpb != 0)sprintf(text, "%s: %d-%d\x3+%d", dtext, dmgl, dmgh, dmgpb);
                                else sprintf(text, "%s: %d-%d", dtext, dmgl, dmgh);
                            }
                        }
                        else sprintf(text, "%s", dtext2);
                        draw_text_original(12, 228, text);


                        bool dv = false;
                        for (int i = 0; i < 255; i++)
                        {
                            if ((m_devotion[i] != 255) && (!dv))
                                dv = devotion_aura(p, m_devotion[i]);
                            else
                                i = 256;
                        }
                        if (dv)//aura buff stuck
                        {
                            char* atext = text_from_tbl(tbl_stats, 3);
                            if (armorb == 0)sprintf(text, "%s: %d\x5+3", atext, armor);
                            else sprintf(text, "%s: %d\x4+%d\x5+3", atext, armor, armorb);
                            draw_text_original(12, 245, text);
                        }
                        else//original armor status
                        {
                            char* atext = text_from_tbl(tbl_stats, 3);
                            if (armorb == 0)sprintf(text, "%s: %d", atext, armor);
                            else sprintf(text, "%s: %d\x4+%d", atext, armor, armorb);
                            draw_text_original(12, 245, text);
                        }

                        char* vtext = text_from_tbl(tbl_stats, 4);
                        sprintf(text, "%s: %d", vtext, vz);
                        draw_text_original(12, 262, text);

                        char* rtext = text_from_tbl(tbl_stats, 5);
                        if (rngb == 0)sprintf(text, "%s: %d", rtext, rng);
                        else sprintf(text, "%s: %d\x4+%d", rtext, rng, rngb);
                        draw_text_original(12 + 72, 262, text);

                        byte pf = *((byte*)((uintptr_t)p + S_PEON_FLAGS));
                        if ((id == U_PEASANT) || (id == U_PEON))
                        {
                            if ((pf & PEON_LOADED) != 0)spdb -= 3;
                        }

                        short hs = *((short*)((uintptr_t)p + S_HASTE));//haste and slow
                        if (hs != 0)
                        {
                            switch (id)
                            {
                            case U_FOOTMAN:case U_GRUNT:case U_GROM:case U_KARGATH:case U_DANATH: case TYPE_B36:case TYPE_B37: case TYPE_A5:case TYPE_A600:
                                if (hs > 0)
                                {
                                    spdb += 3.6;
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 50;
                                }
                                break;
                            case U_PEASANT:case U_PEON:
                                if (hs > 0)
                                {
                                    if ((pf & PEON_LOADED) != 0)spdb += 3;
                                    spdb += 3.6;
                                    if (pch)aspdb += 60;
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 50;
                                }
                                break;
                            case U_ATTACK_PEASANT:case U_ATTACK_PEON:
                                if (hs > 0)
                                {
                                    spdb += 3.6;
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 50;
                                }
                                break;
                            case U_BALLISTA:case U_CATAPULT:
                                if (hs > 0)
                                {
                                    spdb += 4.3;
                                }
                                else
                                {
                                    spdb -= 2.6;
                                    aspdb -= 11;
                                }
                                break;
                            case U_KNIGHT:case U_PALADIN:case U_UTER:case U_TYRALYON:case U_LOTHAR:
                                if (hs > 0)
                                {
                                    spdb += 0.3;
                                }
                                else
                                {
                                    spdb -= 6.8;
                                    aspdb -= 50;
                                }
                                break;
                            case U_OGRE:case U_OGREMAGE:case U_DENTARG:case U_CHOGAL:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 6.8;
                                    aspdb -= 50;
                                }
                                break;
                            case U_ARCHER:case U_RANGER:case U_ALLERIA:
                                if (hs > 0)
                                {
                                    spdb += 3.6;
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 19;
                                }
                                break;
                            case U_TROLL:case U_BERSERK:case U_ZULJIN:
                                if (hs > 0)
                                {
                                    spdb += 3.6;
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 19;
                                }
                                break;
                            case U_MAGE:case U_HADGAR:
                                if (hs > 0)
                                {
                                    spdb += 4.8;
                                    byte odr = *((byte*)((uintptr_t)p + S_ORDER));
                                    if (odr == ORDER_SPELL_BLIZZARD)aspdb += 48;
                                }
                                else
                                {
                                    spdb -= 4.4;
                                    aspdb -= 31;
                                }
                                break;
                            case U_DK:case U_TERON:case U_GULDAN:
                                if (hs > 0)
                                {
                                    spdb += 7.8;
                                    byte odr = *((byte*)((uintptr_t)p + S_ORDER));
                                    if (odr == ORDER_SPELL_ROT)aspdb += 48;
                                }
                                else
                                {
                                    spdb -= 4.4;
                                    aspdb -= 31;
                                }
                                break;
                            case U_DWARWES:
                                if (hs > 0)
                                {
                                    spdb += 5.1;
                                }
                                else
                                {
                                    spdb -= 5.8;
                                    aspdb -= 50;
                                }
                                break;
                            case U_GOBLINS:
                                if (hs > 0)
                                {
                                    spdb += 2.1;
                                }
                                else
                                {
                                    spdb -= 5.8;
                                    aspdb -= 50;
                                }
                                break;
                            case U_GRIFON:case U_KURDRAN:
                                if (hs > 0)
                                {
                                    spdb += 10.5;
                                    aspdb += 1.2;
                                }
                                else
                                {
                                    spdb -= 7.8;
                                    aspdb -= 6.3;
                                }
                                break;
                            case U_DRAGON:case U_DEATHWING:
                                if (hs > 0)
                                {
                                    spdb += 10.5;
                                    aspdb += 1.1;
                                }
                                else
                                {
                                    spdb -= 7.8;
                                    aspdb -= 6.3;
                                }
                                break;
                            case U_HTANKER:case U_OTANKER:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 9;
                                }
                                break;
                            case U_HTRANSPORT:case U_OTRANSPORT:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 5;
                                }
                                break;
                            case U_HDESTROYER:case U_ODESTROYER:case TYPE_A100:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 5;
                                    aspdb -= 10;
                                }
                                break;
                            case U_BATTLESHIP:case U_JUGGERNAUT:
                                if (hs > 0)
                                {
                                    spdb += 4;
                                }
                                else
                                {
                                    spdb -= 3.2;
                                    aspdb -= 5.1;
                                }
                                break;
                            case U_SUBMARINE:case U_TURTLE:
                                if (hs > 0)
                                {
                                    spdb += 3;
                                }
                                else
                                {
                                    spdb -= 3.5;
                                    aspdb -= 12;
                                }
                                break;
                            case U_FLYER:case U_ZEPPELIN:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 7.4;
                                }
                                break;
                            case U_EYE:
                                if (hs > 0)
                                {
                                    //nothing
                                }
                                else
                                {
                                    spdb -= 21;
                                }
                                break;
                            case U_SKELETON:
                                if (hs > 0)
                                {
                                    spdb += 5.8;
                                }
                                else
                                {
                                    spdb -= 3.9;
                                    aspdb -= 35;
                                }
                                break;
                            case U_DEMON:
                                if (hs > 0)
                                {
                                    spdb += 6.9;
                                    aspdb += 19;
                                }
                                else
                                {
                                    spdb -= 7.7;
                                    aspdb -= 29;
                                }
                                break;
                            case U_CRITTER:
                                if (hs > 0)
                                {
                                    spdb += 3.4;
                                }
                                else
                                {
                                    spdb -= 1.6;
                                }
                                break;
                            default:break;
                            }
                        }

                        char* astext1 = text_from_tbl(tbl_stats, 6);
                        char* astext2 = text_from_tbl(tbl_stats, 7);
                        char* astext = astext1;
                        if (pch)astext = astext2;
                        if (ca == 0)
                        {
                            aspd = 0;
                            aspdb = 0;
                        }
                        if (aspdb == 0)sprintf(text, "%s: %d", astext, aspd);
                        else
                        {
                            if (round(aspdb) == aspdb)
                            {
                                if (aspdb > 0)sprintf(text, "%s: %d\x4+%d", astext, aspd, (int)round(aspdb));
                                else sprintf(text, "%s: %d\x3%d", astext, aspd, (int)round(aspdb));
                            }
                            else
                            {
                                if (aspdb > 0)sprintf(text, "%s: %d\x4+%.1f", astext, aspd, aspdb);
                                else sprintf(text, "%s: %d\x3%.1f", astext, aspd, aspdb);
                            }
                        }
                        draw_text_original(12, 279, text);

                        char* stext = text_from_tbl(tbl_stats, 8);
                        if (spdb == 0)sprintf(text, "%s: %d", stext, spd);
                        else
                        {
                            if (round(spdb) == spdb)
                            {
                                if (spdb > 0)sprintf(text, "%s: %d\x4+%d", stext, spd, (int)round(spdb));
                                else sprintf(text, "%s: %d\x3%d", stext, spd, (int)round(spdb));
                            }
                            else
                            {
                                if (spdb > 0)sprintf(text, "%s: %d\x4+%.1f", stext, spd, spdb);
                                else sprintf(text, "%s: %d\x3%.1f", stext, spd, spdb);
                            }
                        }
                        draw_text_original(12, 296, text);

                        if ((mag == 1) || (pch) || ((id == U_BERSERK) && (*(byte*)(GB_MARKS + ow) != 0)) || ((lf != 0) && !check_unit_preplaced(p)))
                        {
                            int y1 = 313;
                            int y2 = 328;
                            if (draw_ranks)
                            {
                                y1 = 296;
                                y2 = 311;
                            }
                            else
                            {
                                if (mag == 1)sprintf(text, "%s:", text_from_tbl(tbl_stats, 9));
                                else
                                {
                                    if (pch)sprintf(text, "%s:", text_from_tbl(tbl_stats, 10));
                                    else if ((id == U_BERSERK) && (*(byte*)(GB_MARKS + ow) != 0))sprintf(text, "%s:", text_from_tbl(tbl_stats, 11));
                                    else if ((lf != 0) && !check_unit_preplaced(p))sprintf(text, "%s:", text_from_tbl(tbl_stats, 12));
                                }
                                draw_text_original(12, 313, text);
                            }
                            byte mp = *((byte*)((uintptr_t)p + S_MANA));
                            byte maxmp = 255;
                            if (pch)
                            {
                                mp = *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS));
                                maxmp = 50;
                            }
                            sprintf(text, "%d", mp);
                            draw_rect(104, y1, 165, y2, 246, true, false);
                            draw_rect(105, y1 + 1, 164, y2 - 1, 239, true, false);
                            draw_rect(106, y1 + 2, 163, y2 - 2, 99, false, false);
                            draw_rect(106, y1 + 2, 106 + (int)floor(57 * ((double)mp / maxmp)), y2 - 2, 186, false, false);
                            draw_text_original(104 + 30 - ((5 * strlen(text)) / 2), y1 + 2, text);
                        }

                        if (draw_ranks)
                        {
                            sprintf(text, "%s:", text_from_tbl(tbl_stats, 13));
                            draw_text_original(12, 313, text);
                            byte xp = *((byte*)((uintptr_t)p + S_KILLS));
                            byte nk = get_next_rank(xp);
                            byte pk = get_prev_rank(xp);
                            sprintf(text, "%d/%d", xp, nk);
                            draw_rect(104, 313, 165, 328, 246, true, false);
                            draw_rect(105, 314, 164, 327, 239, true, false);
                            draw_rect(106, 315, 163, 326, 99, false, false);
                            draw_rect(106, 315, 106 + (int)floor(57 * ((double)(xp - pk) / (nk - pk))), 326, 159, false, false);
                            draw_text_original(104 + 30 - 7 * (strlen(text) / 2), 315, text);
                            grp_make_rank(get_rank_frame(nk));
                            draw_sprite(74 - 15, 314, spr_rank, 28, 14, true, false);
                        }

                        ((void (*)(int, int, int, int))F_INVALIDATE)(5, 165, 168, 331);
                    }
                }
            }
        }
        draw_status = false;
    }
}

void drawing12()
{
    ScreenPTR = getScreenPtr();
    if ((*(byte*)GAME_MODE == 3) && (game_started))
    {
        int* p = NULL;
        for (int i = 0; ((i < 9) && (p == NULL)); i++)
        {
            p = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
            if (p)break;
        }
        if (p)
        {
            byte ow = *((byte*)((uintptr_t)p + S_OWNER));
            if (ow == *(byte*)LOCAL_PLAYER)
            {
                byte id = *((byte*)((uintptr_t)p + S_ID));
                if (id >= U_FARM)
                {
                    WORD uid = unit_fixup(p);
                    int k = 0;
                    while (k <= 7)
                    {
                        if (build_queue[uid * 16 + k * 2] != 255)
                        {
                            DWORD grp = 0;
                            WORD frame = 0xFFFF;
                            byte* spr = spr_portrait_empty;

                            frame = get_default_frame(build_queue[uid * 16 + k * 2], build_queue[uid * 16 + k * 2 + 1]);
                            if (frame != 0xFFFF)grp = global_grp_port;

                            byte era = *(byte*)MAP_ERA;
                            if (era == 0)
                                grp = (DWORD)grp_port_forest;
                            else if (era == 1)
                                grp = (DWORD)grp_port_winter;
                            else if (era == 2)
                                grp = (DWORD)grp_port_wast;
                            else if (era == 3)
                                grp = (DWORD)grp_port_swamp;

                            if (build_queue[uid * 16 + k * 2 + 1] == 1)
                            {
                                if (id == U_HLUMBER)
                                {
                                    if (build_queue[uid * 16 + k * 2] == UG_SP_ROT)
                                    {
                                        grp = (DWORD)grp_saw;
                                        frame = 0;
                                    }
                                    else if (build_queue[uid * 16 + k * 2] == UG_SP_RAISEDEAD)
                                    {
                                        grp = (DWORD)grp_chop;
                                        frame = 0;
                                    }
                                }
                                else if (id == U_FARM)
                                {
                                    if (build_queue[uid * 16 + k * 2] == UG_SP_HASTE)
                                    {
                                        grp = (DWORD)grp_farm;
                                        frame = 0;
                                    }
                                }
                                else if (id == U_STABLES)
                                {
                                    if (build_queue[uid * 16 + k * 2] == UG_SP_RUNES)
                                    {
                                        grp = (DWORD)grp_instr;
                                        frame = 0;
                                    }
                                    else if (build_queue[uid * 16 + k * 2] == UG_SP_BLIZZARD)
                                    {
                                        grp = (DWORD)grp_alebard;
                                        frame = 0;
                                    }
                                    else if (build_queue[uid * 16 + k * 2] == UG_SP_FIRESHIELD)
                                    {
                                        grp = (DWORD)grp_leg;
                                        frame = 0;
                                    }
                                }
                                else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
                                {
                                    if (build_queue[uid * 16 + k * 2] == UG_SP_ARMOR)
                                    {
                                        grp = (DWORD)grp_regen;
                                        frame = 0;
                                    }
                                }
                            }

                            /*
                            if (build_queue[uid * 16 + k * 2 + 1] == 0)
                            {
                                if (build_queue[uid * 16 + k * 2] == U_PEASANT)
                                {
                                    grp = global_grp_port;
                                    frame = 0;
                                }
                                else if (build_queue[uid * 16 + k * 2] == U_FOOTMAN)
                                {
                                    grp = global_grp_port;
                                    frame = 2;
                                }
                            }
                            else if (build_queue[uid * 16 + k * 2 + 1] == 1)
                            {

                            }
                            else if (build_queue[uid * 16 + k * 2 + 1] == 2)
                            {

                            }
                            else if (build_queue[uid * 16 + k * 2 + 1] == 3)
                            {

                            }
                            */

                            if (grp != 0)
                            {
                                WORD mxf = *(WORD*)grp - 1;
                                if (frame <= mxf)
                                {
                                    spr = spr_portrait_small;
                                    grp_make_small_port(grp, frame);
                                }
                            }
                            draw_sprite(10 + k * 19, 291, spr, 23, 19, true, false);
                        }

                        k++;
                    }
                }
            }
        }
    }
}

void drawing3()
{
    ScreenPTR = getScreenPtr();
    if ((*(byte*)GAME_MODE == 3) && (game_started))
    {
        if (can_rain)draw_rain();

        byte cx = *(byte*)CAMERA_X;
        byte cy = *(byte*)CAMERA_Y;

        int* p = NULL;
        for (int i = 0; ((i < 9) && (p == NULL)); i++)
        {
            p = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
            if (p)break;
        }

        if (p)
        {
            byte ow = *((byte*)((uintptr_t)p + S_OWNER));
            if (ow == *(byte*)LOCAL_PLAYER)
            {
                byte id = *((byte*)((uintptr_t)p + S_ID));
                if (id < U_FARM)
                {
                    byte ordr = *((byte*)((uintptr_t)p + S_ORDER));
                    if (!((ordr == ORDER_STOP) || (ordr == ORDER_STAND)))
                    {
                        WORD uid = unit_fixup(p);
                        if (order_queue[uid * 48] != 255)
                        {
                            WORD ux = *((WORD*)((uintptr_t)p + S_DRAW_X)) + 16;
                            WORD uy = *((WORD*)((uintptr_t)p + S_DRAW_Y)) + 16;
                            WORD ox = 0;
                            WORD oy = 0;

                            int* ot = (int*)*((DWORD*)((uintptr_t)p + S_ORDER_UNIT_POINTER));
                            if (ot != NULL)
                            {
                                ox = *((WORD*)((uintptr_t)ot + S_DRAW_X)) + 16;
                                oy = *((WORD*)((uintptr_t)ot + S_DRAW_Y)) + 16;
                            }
                            else
                            {
                                ox = *((WORD*)((uintptr_t)p + S_ORDER_X)) * 32 + 16;
                                oy = *((WORD*)((uintptr_t)p + S_ORDER_Y)) * 32 + 16;
                            }

                            WORD x1 = ux;
                            WORD y1 = uy;
                            WORD x2 = ox;
                            WORD y2 = oy;

                            draw_line_fast_break_safe(m_minx - (cx * 32) + x1, m_miny - (cy * 32) + y1, m_minx - (cx * 32) + x2, m_miny - (cy * 32) + y2, 188, true, 12, 0);
                            if ((x1 != x2) || (y1 != y2))draw_circle_safe(m_minx - (cx * 32) + x2, m_miny - (cy * 32) + y2, 4, 189, false, true);

                            x1 = x2;
                            y1 = y2;

                            int k = 0;
                            while (k <= 15)
                            {
                                if (order_queue[uid * 48 + k * 3] != 255)
                                {
                                    if (order_type_target(order_queue[uid * 48 + k * 3]))
                                    {
                                        int* tr = unit_unfixup((WORD)(order_queue[uid * 48 + k * 3 + 1] + 256 * order_queue[uid * 48 + k * 3 + 2]));
                                        x2 = *((WORD*)((uintptr_t)tr + S_DRAW_X)) + 16;
                                        y2 = *((WORD*)((uintptr_t)tr + S_DRAW_Y)) + 16;
                                    }
                                    else
                                    {
                                        x2 = order_queue[uid * 48 + k * 3 + 1] * 32 + 16;
                                        y2 = order_queue[uid * 48 + k * 3 + 2] * 32 + 16;
                                    }
                                    draw_line_fast_break_safe(m_minx - (cx * 32) + x1, m_miny - (cy * 32) + y1, m_minx - (cx * 32) + x2, m_miny - (cy * 32) + y2, 188, true, 12, 0);
                                    draw_circle_safe(m_minx - (cx * 32) + x1, m_miny - (cy * 32) + y1, 4, 189, false, true);
                                    draw_circle_safe(m_minx - (cx * 32) + x2, m_miny - (cy * 32) + y2, 4, 189, false, true);
                                }
                                x1 = x2;
                                y1 = y2;
                                k++;
                            }
                        }
                    }
                }
                else
                {
                    byte x = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 2));
                    byte y = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 1));
                    byte bp = x & 128;
                    if (bp != 0)
                    {
                        x &= ~128;
                        WORD x1 = *((byte*)((uintptr_t)p + S_X)) * 32 + 16;
                        WORD y1 = *((byte*)((uintptr_t)p + S_Y)) * 32 + 16;
                        WORD x2 = x * 32 + 16;
                        WORD y2 = y * 32 + 16;

                        draw_line_fast_break_safe(m_minx - (cx * 32) + x1, m_miny - (cy * 32) + y1, m_minx - (cx * 32) + x2, m_miny - (cy * 32) + y2 + 12, 188, true, 12, move_line_break);
                        grp_make_rally_flag(0);
                        draw_sprite_safe(m_minx - (cx * 32) + x2 - 16, m_miny - (cy * 32) + y2 - 16, spr_flag, 32, 32, true, true);
                    }
                }
            }
        }
    }
}
//-------------------------------
PROC g_proc_00405463;
void draw_hook1(int a, int b, int c)
{
    ((void (*)(int, int, int))g_proc_00405463)(a, b, c);//original
    drawing12();
    drawing_status();
}

PROC g_proc_00405483;
void draw_hook2(int a, int b, int c)
{
    ((void (*)(int, int, int))g_proc_00405483)(a, b, c);//original
    drawing12();
    drawing_status();
}

PROC g_proc_00421F57;
void draw_hook3()
{
    ((void (*)())g_proc_00421F57)();//original
    drawing3();
}

void statdraw_my_unit(DWORD dlg)
{
    *(byte*)((*(DWORD*)0x004BDC80) + 0x24) = 5;//reset status (grp frame 5 not exists)

    ((void (*)(DWORD))0x00446990)(dlg);//original draw CLEAR all stats, draw orig NAME and HP

    draw_status = true;
}

void statdraw_my_building(DWORD dlg)
{
    *(byte*)((*(DWORD*)0x004BDC80) + 0x24) = 5;//reset status (grp frame 5 not exists)

    int* u = (int*)*(int*)0x004BDC78;//gpStatUnit
    byte ow = *((byte*)((uintptr_t)u + S_OWNER));
    if (ow == *(byte*)LOCAL_PLAYER)
    {
        byte o = *((byte*)((uintptr_t)u + S_ORDER));
        if (o == ORDER_BLDG_BUILD)
        {
            byte bo = *((byte*)((uintptr_t)u + S_BUILD_ORDER));
            WORD f = get_default_frame(*((byte*)((uintptr_t)u + S_BUILD_TYPE)), bo);
            byte id = *((byte*)((uintptr_t)u + S_ID));
            if (id == U_HLUMBER)
            {
                if (f == get_default_frame(UG_SP_ROT, 1))f = 1;
                else if (f == get_default_frame(UG_SP_RAISEDEAD, 1))f = 2;
            }
            else if (id == U_FARM)
            {
                if (f == get_default_frame(UG_SP_HASTE, 1))f = 4;
            }
            else if (id == U_STABLES)
            {
                if (f == get_default_frame(UG_SP_RUNES, 1))f = 1;
                else if (f == get_default_frame(UG_SP_BLIZZARD, 1))f = 2;
                else if (f == get_default_frame(UG_SP_FIRESHIELD, 1))f = 3;
            }
            else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
            {
                if (f == get_default_frame(UG_SP_ARMOR, 1))f = 1;
            }
            ((void (*)(DWORD, WORD))0x0044A710)(dlg, f);//original show_training_port
        }
    }

    ((void (*)(DWORD))0x00447000)(dlg);//original statdraw_draw_bldg

    draw_status = true;
}

//004A3D58 portrait cards (icon 2b frame_type 2b str_id 4b function1 4b function2 4b)
void statdraw_init()
{
    char buf[] = "\x90\x90\x90\x90\x90\x90\x90";
    PATCH_SET((char*)0x00446A6A, buf);//no draw level in orig draw clear
    char buf2[] = "\x90\x90";
    PATCH_SET((char*)0x00446B43, buf2);//no draw completion bar
    char buf3[] = "\x90\x90\x90\x90\x90";
    PATCH_SET((char*)0x00447076, buf3);//no clear bldg

    for (int i = U_FOOTMAN; i < U_FARM; i++)patch_setdword((DWORD*)(0x004A3D58 + 16 * i + 12), (DWORD)statdraw_my_unit);
    for (int i = U_FARM; i < U_OWALL; i++)patch_setdword((DWORD*)(0x004A3D58 + 16 * i + 12), (DWORD)statdraw_my_building);
}

PROC g_proc_0041FD16;
void seq_free()
{
    ((void (*)())g_proc_0041FD16)();//original
    game_started = false;
}

WORD mouse_x = 0;
WORD mouse_y = 0;
#define EVENTS_MOUSE_STATUS 0x004D5134
#define EVENTS_INFO 0x004D5134

void events_main(WORD mx, WORD my, WORD ev, byte key)
{
    mouse_x = mx;
    mouse_y = my;
}

PROC g_proc_0040585E;
int events_menu(int a, int b)
{
    int original = ((int (*)(int, int))g_proc_0040585E)(a, b);//original
    WORD mx = *(WORD*)(EVENTS_INFO);
    WORD my = *(WORD*)(EVENTS_INFO + 2);
    WORD ev = *(WORD*)a;
    byte key = *(byte*)(a + 12);
    events_main(mx, my, ev, key);
    return original;
}

PROC g_proc_0041C763;
int events_game(int a, int b)
{
    int original = ((int (*)(int, int))g_proc_0041C763)(a, b);//original
    WORD mx = *(WORD*)(EVENTS_MOUSE_STATUS);
    WORD my = *(WORD*)(EVENTS_MOUSE_STATUS + 2);
    WORD ev = *(WORD*)a;
    byte key = *(byte*)(a + 12);
    events_main(mx, my, ev, key);
    return original;
}

byte spr_cursor0[23 * 30] = {
108,103,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
103,103,100,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 98,100,108,103,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,100,103,103,100,  0,100,113,100,  0,112,100,  0,108,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0, 98,100,100,113,103,100,108,103,100,108,103,100,103,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0, 98, 98,108,108,100,100,100,100,100, 98, 98, 98,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0, 98,100,103,100,102,103,105,105,105,105,103,103,101,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,100,100,107,108,105,105,105,105,103,102,101,102,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 98,100,113,105,103,105,105,105,102,102,102,101,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,100,108,105,103,103,105,105,102,102,102,102,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,103,100, 98,103,108,103,103,103,105,102,102,102,100,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,108,103,100,100,105,105,103,103,103,102,101,100, 98,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,100,103,103, 98,100,105,103,103,107,108,112,113,112,108,  0,  0,  0,  0,  0,  0,  0,
  0,  0, 98,100,100,100, 98,100,103,108,105,103,102,100, 98,100,100,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0, 98,100, 98,102,108,103,103,103,103,105,100,100,102,102,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 98, 98,105,103,100,103,103,103,103,107,100,102,102,102,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0, 97,103,100,100,105,103,103,103,105,108,100,102,103,100,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0, 98,100, 98,103,105,103,103,103,107,109,102,102,103, 98,  0,  0,
  0,  0,  0,  0,  0,  0,  0,100, 97, 98, 98,108,103,103,103,105,108,109,102,103,100, 98,  0,
  0,  0,  0,  0,  0,  0,  0, 97, 98, 98, 99,100,108,105,103,105,105,112,109,105,100,100, 97,
  0,  0,  0,  0,  0,  0,  0,  0, 99, 99,100,101,100,105,105,105,105,109,112,109, 98,101, 98,
  0,  0,  0,  0,  0,  0,  0,  0,100, 99,101,101, 98, 97,101,105,108,113,113,113, 97,101, 98,
  0,  0,  0,  0,  0,  0,  0,  0,101, 99,102, 98,239,239,239,239, 97,100,100,102, 97,102, 98,
  0,  0,  0,  0,  0,  0,  0,  0,101,100,102,239,239,239,239,239,239, 97, 97, 97, 97,102, 98,
  0,  0,  0,  0,  0,  0,  0,  0,101,101,103,239,239,239,239,239,239,239, 97, 97, 97,101, 98,
  0,  0,  0,  0,  0,  0,  0,  0,100,101,107,239,239,239,239,239,239,239,239, 97, 99,100, 97,
  0,  0,  0,  0,  0,  0,  0,  0, 97,101,105,101,239,239,239,239,239,239,239, 99,102, 98,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0, 99,101,103,101, 97,239,239,239, 99,101,103, 99,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 98,100,101,101,101,102,102,103,101, 98,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 97, 98, 99, 99, 99, 98, 97,  0,  0,  0,  0,
};

byte spr_cursor1[23 * 30] = {
  0,128,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
128,121,117,128,  0,  0,  0,  0,101, 99,  0,  0,  0,  0, 99,101,  0,  0,  0,  0,  0,  0,  0,
128,119,121,115,128,101, 97,  0, 99,113,101, 97, 97, 97,113, 99,  0,  0,  0,  0,  0,  0,  0,
128,115,118,119, 99,113,110, 97, 97,110,113,104,103,106,109, 97,  0,  0,  0,  0,  0,  0,  0,
  0,128,116,118, 99,110,107,107,106,100,101,103,103,100,101,102, 97,  0,  0,  0,  0,  0,  0,
  0,128,114,117, 99,100,101,107,106,101,104,103,103,101,102,101, 97,  0,  0,  0,  0,  0,  0,
  0,  0,128,116, 97,101,107,107,106,105,113,109,102,102,102,106,113, 97,  0,  0,  0,  0,  0,
  0,  0,128, 97,113,105,113,111,106,105,109,105,102,102,102,105, 99,  0,  0,  0,  0,  0,  0, 
  0,  0,128,135, 97,105,105,110,106,105,100,101,102,101, 99,101, 97,  0,  0,  0,  0,  0,  0,
  0,  0,128,134, 97,104,101,100,105,105,101,103,113,101,101, 99,105, 97,  0,  0,  0,  0,  0,
  0,  0,128,131,131, 97,103,101,105,105,103,102,102, 99, 99, 98, 98, 97,  0,  0,  0,  0,  0,
  0,  0,128,135,134, 97,102,103,103,109,103, 99, 98,136,138,141,120,139,128,  0,  0,  0,  0,
  0,  0,128,134,135, 97,113,102,102, 99, 98,135,138,140,140,138,138,136,133,128,  0,  0,  0,
  0,  0,128,132,134,131, 97, 98,102, 98,136,140,139,138,134,134,136,136,133,128,  0,  0,  0,
  0,  0,128,131,131,132,131, 97,131,136,139,138,136,135,135,122,134,133,133,128,  0,  0,  0,
  0,  0,  0,128,128,131,131,131,133,136,136,135,135,135,133,120,116,119,114,114,128,  0,  0,
  0,  0,  0,  0,  0,128,131,131,133,136,135,135,135,133,114,118,119,122,116,119,122,128,  0,
  0,  0,  0,  0,  0,128,136,131,131,135,135,131,114,116,120,116,116,134,135,136,135,134,128,
  0,  0,  0,  0,  0,128,136,134,131,133,133,122,118,120,122,136,136,138,137,137,137,137,133,
  0,  0,  0,  0,  0,128,131,134,135,131,116,120,133,136,139,139,138,138,137,136,137,138,138,
  0,  0,  0,  0,  0,  0,128,131,134,114,120,131,131,136,139,140,139,138,137,137,136,137,139,
  0,  0,  0,  0,  0,  0,  0,128,114,118,114,131,135,136,138,120,141,139,137,136,136,136,132,
  0,  0,  0,  0,  0,  0,  0,128,119,122,131,131,131,133,136,135,134,133,133,132,132,131,131,
  0,  0,  0,  0,  0,  0,  0,128,131,133,134,131,239, 97,101,100,100,100, 99, 99, 97, 97,131,
  0,  0,  0,  0,  0,  0,128,131,134,134,136,131,239,239, 97,100,102,105,105,102,101,100, 97,
  0,  0,  0,  0,  0,  0,128,134,134,136,131,239,239,239,239,239,239,239,239,239,239, 97, 97,
  0,  0,  0,  0,  0,  0,128,133,133,131,239,239,239,239,239,239,239,239,239,239,239,239, 97,
  0,  0,  0,  0,  0,  0,  0,128,128,  0, 97,239,239,239,239,239,239,239,239,239,239, 99, 99, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 97, 97,239,239,239,239,239,239,239, 97, 99, 99,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 99, 99, 97,239,239,239, 97, 99, 99, 97,  0,  0,
};

#define W2_CURSOR_ID *((unsigned char*)0x004AE098)

PROC g_proc_004895E8;
void draw_cursor(int a, int b, int c, int d)
{
    WORD mx = *(WORD*)(EVENTS_MOUSE_STATUS);
    WORD my = *(WORD*)(EVENTS_MOUSE_STATUS + 2);
    if (W2_CURSOR_ID == 0)
    {
        draw_sprite_400slower_safe_screen(mx, my, spr_cursor0, 23, 30, true, true);
    }
    else if (W2_CURSOR_ID == 1)
    {
        draw_sprite_400slower_safe_screen(mx, my, spr_cursor1, 23, 30, true, true);
    }
    else ((void (*)(int, int, int, int))g_proc_004895E8)(a, b, c, d);//original
}

void latehook(int adr, PROC* p, char* func)
{
    if (*p == NULL)*p = patch_call((char*)adr, func);
}

PROC g_proc_0042A4A1;
int new_game(int* loaded_game, int b, long c)
{
    latehook(0x004453A7, &g_proc_late_004453A7, (char*)grp_draw_portrait2);

    game_started = false;
    if (loaded_game == NULL)
    {
        sfile_read_set_default();
    }
    autosave_counter = 0;
    buttons_init_all();

    if (remember_music != 101)
        *(DWORD*)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD*)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM); // set volume
    remember_music = 101;
    remember_sound = 101;

    a_custom = b % 256; // custom game or campaign
    if (a_custom)
        *(byte*)LEVEL_OBJ = 53; // remember custom obj
    else
    {
        if (*(byte*)LEVEL_OBJ == 53)
            a_custom = 1; // fix for when saved game loads custom get broken
    }
    replace_trigger();
    memset((void*)GB_HORSES, 0, 16 * sizeof(byte));
    int original = ((int (*)(int*, int, long))g_proc_0042A4A1)(loaded_game, b, c); // original
    if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_HUMAN2) || (*(byte*)LEVEL_OBJ == LVL_HUMAN3)
        || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN2) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN3)
        || (*(byte*)LEVEL_OBJ == LVL_ORC1))
        tilesets_change_fog(239);
    else
        tilesets_change_fog(210);

    for (int i = 0; i < RAINDROPS; i++)
    {
        raindrops[i].l = 0;
    }
    can_rain = false;
    byte lvl = *(byte*)LEVEL_OBJ;
    if (lvl == LVL_ORC1)
        set_rain(200, 4, 2, 8, 5, 6, true, 0);

    red_at = false;
    green_at = false;
    violet_at = false;

    srand(time(0));

    if (loaded_game == NULL)
    {
        find_all_alive_units(U_ZULJIN);
        set_random_xp_all(20, 28);
        find_all_alive_units(U_KARGATH);
        set_random_xp_all(20, 28);
        find_all_alive_units(U_TROLL);
        set_random_xp_all(1, 3);
        find_all_alive_units(U_GROM);
        set_random_xp_all(1, 7);
        find_all_alive_units(U_BERSERK);
        set_random_xp_all(4, 10);
        find_all_alive_units(U_DENTARG);
        set_random_xp_all(13, 19);
        find_all_alive_units(U_GRUNT);
        set_random_xp_all(1, 8);
        find_all_alive_units(U_CATAPULT);
        set_random_xp_all(1, 8);
        find_all_alive_units(U_OGRE);
        set_random_xp_all(3, 12);
        find_all_alive_units(U_ODESTROYER);
        set_random_xp_all(1, 7);
        find_all_alive_units(U_TURTLE);
        set_random_xp_all(3, 19);
        find_all_alive_units(U_DK);
        set_random_xp_all(26, 30);
    }

    return original;
}

PROC g_proc_0041F7E4;
int load_game(int a)
{
    int original = ((int (*)(int))g_proc_0041F7E4)(a); // original
    replace_trigger();

    return original;
}

void hook(int adr, PROC *p, char *func)
{
    *p = patch_call((char *)adr, func);
}

void common_hooks()
{
    hook(0x0045271B, &g_proc_0045271B, (char *)update_spells);
    hook(0x004522B9, &g_proc_004522B9, (char *)seq_run);

    hook(0x0041038E, &g_proc_0041038E, (char *)damage1);
    hook(0x00409F3B, &g_proc_00409F3B, (char *)damage2);
    hook(0x0040AF70, &g_proc_0040AF70, (char *)damage3);
    hook(0x0040AF99, &g_proc_0040AF99, (char *)damage4);
    hook(0x00410762, &g_proc_00410762, (char *)damage5);
    hook(0x004428AD, &g_proc_004428AD, (char *)damage6);

    hook(0x0043BAE1, &g_proc_0043BAE1, (char *)rc_snd);
    hook(0x0043B943, &g_proc_0043B943, (char *)rc_build_click);
    hook(0x0040DF71, &g_proc_0040DF71, (char *)bld_unit_create);
    hook(0x0040AFBF, &g_proc_0040AFBF, (char *)tower_find_attacker);
    hook(0x00451728, &g_proc_00451728, (char *)unit_kill_deselect);

    hook(0x00455F9C, &g_proc_00455F9C, (char*)bldg_build_start_packet);
    hook(0x0047632D, &g_proc_0047632D, (char*)bldg_build_start_packet);

    hook(0x00455EA2, &g_proc_00455EA2, (char*)unit_set_target);
    hook(0x0043B943, &g_proc_0043B943_2, (char*)unit_set_target2);
    hook(0x0043B116, &g_proc_0043B116, (char*)unit_set_target);

    hook(0x0043B0EF, &g_proc_0043B0EF, (char*)find_bldg_entry_corner);

    hook(0x0042479E, &g_proc_0042479E, (char *)peon_into_goldmine);
    hook(0x00424745, &g_proc_00424745, (char *)goods_into_inventory);
    hook(0x004529C0, &g_proc_004529C0, (char *)goods_into_inventory);
    hook(0x00451054, &g_proc_00451054, (char *)count_add_to_tables_load_game);
    hook(0x00438A5C, &g_proc_00438A5C, (char *)unset_peon_ai_flags);
    hook(0x00438985, &g_proc_00438985, (char *)unset_peon_ai_flags);

    hook(0x0040EEDD, &g_proc_0040EEDD, (char *)upgrade_tower);
    hook(0x00442E25, &g_proc_00442E25, (char *)create_skeleton);
    hook(0x00425D1C, &g_proc_00425D1C, (char *)cast_raise);
    hook(0x00424F94, &g_proc_00424F94, (char*)cast_runes);
    hook(0x00424FD7, &g_proc_00424FD7, (char*)cast_runes);
    hook(0x0042757E, &g_proc_0042757E, (char *)ai_spell);
    hook(0x00427FAE, &g_proc_00427FAE, (char *)ai_attack);
    hook(0x00427FFF, &g_proc_00427FFF, (char *)ai_attack_nearest_enemy);
	hook(0x00437679, &g_proc_00437679, (char*)in_range);

    hook(0x0043C5F5, &g_proc_0043C5F5, (char*)sfile_read);
    hook(0x0043D528, &g_proc_0043D528, (char*)sfile_write);
    hook(0x0043CFF2, &g_proc_0043CFF2, (char*)sfile_write);

    hook(0x0042049F, &g_proc_0042049F, (char *)game_end_dialog);
    hook(0x0042A4A1, &g_proc_0042A4A1, (char *)new_game);
    hook(0x0041F7E4, &g_proc_0041F7E4, (char *)load_game);

    hook(0x0041FD16, &g_proc_0041FD16, (char*)seq_free);

    hook(0x00421E6F, &g_proc_00421E6F, (char *)tileset_draw_full_fog);
    hook(0x00421E81, &g_proc_00421E81, (char *)tileset_draw_fog_of_war);
    hook(0x00421242, &g_proc_00421242, (char *)tileset_draw_fog_minimap);

    hook(0x0040585E, &g_proc_0040585E, (char*)events_menu);
    hook(0x0041C763, &g_proc_0041C763, (char*)events_game);
}

void rocks_hooks()
{
    patch_ljmp((char *)0x0044E74B, (char *)finish_not_function);
    char buf[] = "\x57\x66\x8B\x4\x48\x66\x3D\xFD\xFF\x74\x0A\x66\x3D\xFF\xFF\xF\x85\x0\x0\x0\x0\xE9\x0\x0\x0\x0";
    PATCH_SET((char *)finish_not_function, buf);
    patch_ljmp((char *)finish_not_function + 16, (char *)0x0044E835); // return
    patch_ljmp((char *)finish_not_function + 21, (char *)0x0044E758); // continue
    char buf2[] = "\x85";
    PATCH_SET((char *)finish_not_function + 16, buf2); // repair jnz

    hook(0x0043B888, &g_proc_0043B888, (char *)rclick_get_unit_first);
    hook(0x0043B94B, &g_proc_0043B94B, (char *)rclick_get_unit_next);
    hook(0x0043B779, &g_proc_0043B779, (char *)rclick_tile_is_chopping_tree);
    hook(0x0043B768, &g_proc_0043B768, (char *)rclick_tile_is_tree);
    hook(0x0042411E, &g_proc_0042411E, (char *)dispatch_tile_is_chopping_tree);
    hook(0x0042412B, &g_proc_0042412B, (char *)dispatch_tile_cancel_tree_harvest);
    hook(0x00451554, &g_proc_00451554, (char *)dispatch_tile_cancel_tree_harvest);
    hook(0x00453001, &g_proc_00453001, (char *)dispatch_tile_cancel_tree_harvest);
    hook(0x0045307D, &g_proc_0045307D, (char *)dispatch_tile_cancel_tree_harvest);
    hook(0x004241AD, &g_proc_004241AD, (char *)dispatch_tile_finish_tree_harvest);
    hook(0x00424215, &g_proc_00424215, (char *)dispatch_tile_is_tree);
    hook(0x0042423D, &g_proc_0042423D, (char *)dispatch_tile_start_tree_harvest);
    hook(0x0042413C, &g_proc_0042413C, (char *)dispatch_tile_find_tree);
    hook(0x00424262, &g_proc_00424262, (char *)dispatch_tile_find_tree);
    hook(0x004377EC, &g_proc_004377EC, (char *)dispatch_tile_find_tree);
}

void files_hooks()
{
    files_init();

    hook(0x004542FB, &g_proc_004542FB, (char *)grp_draw_cross);
    hook(0x00454DB4, &g_proc_00454DB4, (char *)grp_draw_bullet);
    hook(0x00454BCA, &g_proc_00454BCA, (char *)grp_draw_unit);
    hook(0x00455599, &g_proc_00455599, (char *)grp_draw_building);
    hook(0x0043AE54, &g_proc_0043AE54, (char *)grp_draw_building_placebox);
    hook(0x0044538D, &g_proc_0044538D, (char *)grp_portrait_init);
    hook(0x004453A7, &g_proc_004453A7, (char *)grp_draw_portrait);
    hook(0x004452B0, &g_proc_004452B0, (char *)grp_draw_portrait_buttons);
    hook(0x0044A65C, &g_proc_0044A65C, (char *)status_get_tbl);
    hook(0x0044AC83, &g_proc_0044AC83, (char *)unit_hover_get_id);
    hook(0x0044AE27, &g_proc_0044AE27, (char *)unit_hover_get_tbl);
    hook(0x0044AE56, &g_proc_0044AE56, (char *)button_description_get_tbl);
    hook(0x0044B23D, &g_proc_0044B23D, (char *)button_hotkey_get_tbl);
    hook(0x004354C8, &g_proc_004354C8, (char *)objct_get_tbl_custom);
    hook(0x004354FA, &g_proc_004354FA, (char *)objct_get_tbl_campanign);
    hook(0x004300A5, &g_proc_004300A5, (char *)objct_get_tbl_briefing_task);
    hook(0x004300CA, &g_proc_004300CA, (char *)objct_get_tbl_briefing_title);
    hook(0x004301CA, &g_proc_004301CA, (char *)objct_get_tbl_briefing_text);
    hook(0x00430113, &g_proc_00430113, (char *)objct_get_briefing_speech);
    hook(0x0041F0F5, &g_proc_0041F0F5, (char *)finale_get_tbl);
    hook(0x0041F1E8, &g_proc_0041F1E8, (char *)finale_credits_get_tbl);
    hook(0x0041F027, &g_proc_0041F027, (char *)finale_get_speech);
    hook(0x00417E33, &g_proc_00417E33, (char *)credits_small_get_tbl);
    hook(0x00417E4A, &g_proc_00417E4A, (char *)credits_big_get_tbl);
    hook(0x0042968A, &g_proc_0042968A, (char *)act_get_tbl_small);
    hook(0x004296A9, &g_proc_004296A9, (char *)act_get_tbl_big);
    hook(0x0041C51C, &g_proc_0041C51C, (char *)netstat_get_tbl_nation);
    hook(0x00431229, &g_proc_00431229, (char *)rank_get_tbl);
    hook(0x004372EE, &g_proc_004372EE, (char *)pcx_load_menu);
    hook(0x00430058, &g_proc_00430058, (char *)pcx_load_briefing);
    hook(0x00429625, &g_proc_00429625, (char *)pcx_load_act);
    hook(0x00429654, &g_proc_00429654, (char *)pcx_load_act);
    hook(0x0041F004, &g_proc_0041F004, (char *)pcx_load_final);
    hook(0x00417DDB, &g_proc_00417DDB, (char *)pcx_load_credits);
    hook(0x0043169E, &g_proc_0043169E, (char *)pcx_load_statistic);
    hook(0x00462D4D, &g_proc_00462D4D, (char *)storm_file_load);
    hook(0x0041F9FD, &g_proc_0041F9FD, (char *)tilesets_load);

    hook(0x0041F97D, &g_proc_0041F97D, (char *)map_file_load);

    hook(0x0042A443, &g_proc_0042A443, (char *)act_init);

    hook(0x0044F37D, &g_proc_0044F37D, (char *)main_menu_init);

    hook(0x0043B16F, &g_proc_0043B16F, (char *)smk_play_sprintf_name);
    hook(0x0043B362, &g_proc_0043B362, (char *)smk_play_sprintf_blizzard);

    hook(0x00440F4A, &g_proc_00440F4A, (char *)music_play_get_install);
    hook(0x00440F5F, &g_proc_00440F5F, (char *)music_play_get_install);
    patch_call((char *)0x00440F41, (char *)music_play_sprintf_name);
    char buf[] = "\x90";
    PATCH_SET((char *)(0x00440F41 + 5), buf); // 7 bytes call

    hook(0x00424A9C, &g_proc_00424A9C, (char *)storm_font_load);
    hook(0x00424AB2, &g_proc_00424AB2, (char *)storm_font_load);
    hook(0x004288B2, &g_proc_004288B2, (char *)storm_font_load);
    hook(0x00428896, &g_proc_00428896, (char *)storm_font_load);
    hook(0x0042887D, &g_proc_0042887D, (char *)storm_font_load);

    hook(0x00422D76, &g_proc_00422D76, (char *)sound_play_unit_speech);
    hook(0x00422D5F, &g_proc_00422D5F, (char *)sound_play_unit_speech_soft);
}

void capture_fix()
{
    char buf[] = "\xB0\x01\xF6\xC1\x02\x74\x02\xB0\x02\x50\x66\x8B\x7B\x18\x66\x8B\x6B\x1A\x8B\xD7\x8B\xF5\x29\xC2\x29\xC6\x8D\x43\x27\x31\xC9\x89\x44\x24\x24\x8A\x08\xC1\xE1\x02\x66\x8B\x81\x1C\xEE\x4C\x00\x66\x8B\x89\x1E\xEE\x4C\x00\x66\x01\xF8\x66\x01\xE9\x5D\x01\xE8\x01\xE9\x90\x90";
    PATCH_SET((char *)CAPTURE_BUG, buf);
}

void fireshield_cast_fix()
{
    char buf[] = "\x90\x90";
    PATCH_SET((char *)FIRE_CAST1, buf);
    char buf2[] = "\x90\x90\x90\x90\x90\x90";
    PATCH_SET((char *)FIRE_CAST2, buf2);
}

char save_folder[] = "Save\\RedMistPrequel\\";

void change_save_folder()
{
    patch_setdword((DWORD *)0x0043C07A, (DWORD)save_folder);
    patch_setdword((DWORD *)0x0043CE66, (DWORD)save_folder);
    patch_setdword((DWORD *)0x0043D2C1, (DWORD)save_folder);
    patch_setdword((DWORD *)0x0043D383, (DWORD)save_folder);
    patch_setdword((DWORD *)0x0043FB66, (DWORD)save_folder);
}

extern "C" __declspec(dllexport) void w2p_init()
{
    DWORD check_exe = *(DWORD *)0x48F2F0;
    if (check_exe == 0x51504d52) // RMPQ
    {
        LPCTSTR sk = TEXT("SOFTWARE\\Battle.net\\Characters");
        LPCTSTR value = TEXT("Names");
        DWORD gsz = MAX_PATH;
        char getdata[MAX_PATH];
        HKEY hKey;
        LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
        if (openRes == ERROR_SUCCESS)
        {
            openRes = RegQueryValueExA(hKey, value, NULL, NULL, (byte*)getdata, &gsz);
            if (openRes == ERROR_SUCCESS)
            {
                if (strlen(getdata) <= 28)
                {
                    for (int i = 0; i < 32; i++)hero_name[i] = '\x0';
                    for (int i = 0; i < strlen(getdata); i++)hero_name[i] = getdata[i];
                    hero_name_loaded = true;
                }
            }
        }
        RegCloseKey(hKey);

        m_screen_w = *(WORD*)SCREEN_SIZE_W;
        m_screen_h = *(WORD*)SCREEN_SIZE_H;
        m_added_width = m_screen_w - 640;
        m_added_height = m_screen_h - 480;
        m_align_x = m_added_width > 0 ? m_added_width / 2 : 0;
        m_align_y = m_added_height > 0 ? m_added_height / 2 : 0;
        m_maxx = m_minx + m_screen_w - 200;
        m_maxy = m_miny + m_screen_h - 40;

        hook(0x00405463, &g_proc_00405463, (char*)draw_hook1);
        hook(0x00405483, &g_proc_00405483, (char*)draw_hook2);
        hook(0x00421F57, &g_proc_00421F57, (char*)draw_hook3);

        common_hooks();
        rocks_hooks();
        files_hooks();

        buttons_orig();
        buttons_not_change_status();
        statdraw_init();
        char buf[] = "\xEB";
        PATCH_SET((char*)0x0044AB5B, buf);//hotkey work with shift

        hook(0x0042BB04, &g_proc_0042BB04, (char*)map_create_unit);
        hook(0x00418937, &g_proc_00418937, (char*)dispatch_die_unitdraw_update_1_man);

        hook(0x0042A466, &g_proc_0042A466, (char*)briefing_check);
        hook(0x00416930, &g_proc_00416930, (char*)player_race_mission_cheat);
        hook(0x0042AC6D, &g_proc_0042AC6D, (char*)player_race_mission_cheat2);

        hook(0x00409F1E, &g_proc_00409F1E, (char*)damage_calc_damage_action);
        hook(0x0040FBE7, &g_proc_0040FBE7, (char*)damage_calc_damage_bullet);
        hook(0x0040FBE0, &g_proc_0040FBE0, (char*)damage_calc_sum);
        hook(0x00410733, &g_proc_00410733, (char*)damage_area_get_target_armor);
        hook(0x0040AFD4, &g_proc_0040AFD4, (char*)damage_bullet_tower_create);

        hook(0x0043789D, &g_proc_0043789D, (char*)unit_next_action);
        char bufact[] = "\x90\x90\x90\x90\x90";
        PATCH_SET((char*)0x0041905F, bufact);//remove unit_set_next_action from dispatch_create_bldg

        sounds_tables();

        char bufres[] = "\x90\x90\x90";
        PATCH_SET((char*)0x0045158A, bufres);//peon die with res sprite bug fix
        hook(0x00451590, &g_proc_00451590, (char*)unit_kill_peon_change);

        // char buf_showpath[] = "\xB0\x1\x90\x90\x90";
        // PATCH_SET((char*)0x00416691, buf_showpath);
        hook(0x0045614E, &g_proc_0045614E, (char*)receive_cheat_single);

        *(byte*)(0x0049D93C) = 0;                          // remove text
        *(byte*)(0x0049DC24) = 0;                          // from main menu
        patch_call((char*)0x004886B3, (char*)0x0048CCA2); // remove fois HD text from main menu

        char lc[] = "LC.dll";
        int ad = (int)GetModuleHandleA(lc);
        if (ad != 0)
        {
            ad += 0x5240;
            if (*(byte*)ad == 4)
            {
                char buf[] = "\xff";
                PATCH_SET((char*)ad, buf);
            }
        }

        char buf_i[] = "\xE8\x1B\x31\x5\x0";
        PATCH_SET((char*)0x00428E08, buf_i); // fix fois install

        GetCurrentDirectoryA(MAX_PATH, my_dir);
        sprintf(autosave_named, "%s\\%sautosave.sav", my_dir, save_folder);
        sprintf(autosave_name, "autosave");
        change_save_folder();

        capture_fix();
        fireshield_cast_fix();

        buttons_orig();

        /* hook cursor drawing */
        //if (GetModuleHandleA("HardwareCursor.w2p") == NULL)
        //{
            //patch_setdword((DWORD*)0x00428931, (DWORD)draw_cursor);
            //patch_setdword((DWORD*)0x004D48B8, (DWORD)draw_cursor);
            //hook(0x004895E8, &g_proc_004895E8, (char*)draw_cursor);
        //}

        char buf_unload_check[] = "\x0\x0\x0\x0";
        PATCH_SET((char*)0x0048F2F0, buf_unload_check); // dll unload
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID)
{
    return TRUE;
}