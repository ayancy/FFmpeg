/*
 * Generates a synthetic YUV video sequence suitable for codec testing.
 * GPLv2
 * rotozoom.c -> s.bechet@av7.net
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SCALEBITS 8
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)		((int) ((x) * (1L<<SCALEBITS) + 0.5))
typedef unsigned char UINT8;

static void rgb24_to_yuv420p(UINT8 *lum, UINT8 *cb, UINT8 *cr,
                              UINT8 *src, int width, int height)
{
    int wrap, wrap3, x, y;
    int r, g, b, r1, g1, b1;
    UINT8 *p;

    wrap = width;
    wrap3 = width * 3;
    p = src;
    for(y=0;y<height;y+=2) {
        for(x=0;x<width;x+=2) {
            r = p[0];
            g = p[1];
            b = p[2];
            r1 = r;
            g1 = g;
            b1 = b;
            lum[0] = (FIX(0.29900) * r + FIX(0.58700) * g + 
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            r = p[3];
            g = p[4];
            b = p[5];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[1] = (FIX(0.29900) * r + FIX(0.58700) * g + 
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            p += wrap3;
            lum += wrap;

            r = p[0];
            g = p[1];
            b = p[2];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[0] = (FIX(0.29900) * r + FIX(0.58700) * g + 
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            r = p[3];
            g = p[4];
            b = p[5];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[1] = (FIX(0.29900) * r + FIX(0.58700) * g + 
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            
            cb[0] = ((- FIX(0.16874) * r1 - FIX(0.33126) * g1 + 
                      FIX(0.50000) * b1 + 4 * ONE_HALF - 1) >> (SCALEBITS + 2)) + 128;
            cr[0] = ((FIX(0.50000) * r1 - FIX(0.41869) * g1 - 
                     FIX(0.08131) * b1 + 4 * ONE_HALF - 1) >> (SCALEBITS + 2)) + 128;

            cb++;
            cr++;
            p += -wrap3 + 2 * 3;
            lum += -wrap + 2;
        }
        p += wrap3;
        lum += wrap;
    }
}

/* cif format */
#define DEFAULT_WIDTH   352
#define DEFAULT_HEIGHT  288
#define DEFAULT_NB_PICT 360

void pgmyuv_save(const char *filename, int w, int h,
                 unsigned char *rgb_tab)
{
    FILE *f;
    int i, h2, w2;
    unsigned char *cb, *cr;
    unsigned char *lum_tab, *cb_tab, *cr_tab;

    lum_tab = malloc(w * h);
    cb_tab = malloc((w * h) / 4);
    cr_tab = malloc((w * h) / 4);

    rgb24_to_yuv420p(lum_tab, cb_tab, cr_tab, rgb_tab, w, h);

    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", w, (h * 3) / 2, 255);
    fwrite(lum_tab, 1, w * h, f);
    h2 = h / 2;
    w2 = w / 2;
    cb = cb_tab;
    cr = cr_tab;
    for(i=0;i<h2;i++) {
        fwrite(cb, 1, w2, f);
        fwrite(cr, 1, w2, f);
        cb += w2;
        cr += w2;
    }
    fclose(f);

    free(lum_tab);
    free(cb_tab);
    free(cr_tab);
}

unsigned char *rgb_tab;
int width, height, wrap;

void put_pixel(int x, int y, int r, int g, int b)
{
    unsigned char *p;

    if (x < 0 || x >= width ||
        y < 0 || y >= height)
        return;

    p = rgb_tab + y * wrap + x * 3;
    p[0] = r;
    p[1] = g;
    p[2] = b;
}

unsigned char tab_r[256*256];
unsigned char tab_g[256*256];
unsigned char tab_b[256*256];

int teta = 0;
int h_cos [360];
int h_sin [360];

void gen_image(int num, int w, int h)
{
  const int c = h_cos [teta];
  const int s = h_sin [teta];
  
  const int xi = -(w/2) * c;
  const int yi =  (w/2) * s;
  
  const int xj = -(h/2) * s;
  const int yj = -(h/2) * c;
  
  unsigned dep;
  int i,j;
  
  int x,y;
  int xprime = xj;
  int yprime = yj;


  for (j=0;j<h;j++) {

    x = xprime + xi;
    xprime += s;

    y = yprime + yi;
    yprime += c;
      
    for ( i=0 ; i<w ; i++ ) {
      x += c;
      y -= s;
      dep = ((x>>8)&255) + (y&(255<<8));
      put_pixel(i, j, tab_r[dep], tab_g[dep], tab_b[dep]);
    }
  }
  teta = (teta+1) % 360;
}

void init_demo() {
  int i,j;
  double h;
  double radian;
  char line[3 * 128];

  FILE *fichier;


  fichier = fopen("rotozoom-ffmpeg.pnm","r");
  fread(line, 1, 15, fichier);
  for (i=0;i<128;i++) {
    fread(line,1,3*128,fichier);
    for (j=0;j<128;j++) {
	  tab_r[256*i+j] = line[3*j    ];
	  tab_g[256*i+j] = line[3*j + 1];
	  tab_b[256*i+j] = line[3*j + 2];
    }
	memcpy(tab_r + 257*128 + i*256, tab_r + i*256, 128);
	memcpy(tab_g + 257*128 + i*256, tab_g + i*256, 128);
	memcpy(tab_b + 257*128 + i*256, tab_b + i*256, 128);
  }
  fclose(fichier);

  fichier = fopen("rotozoom-tux.pnm","r");
  fread(line, 1, 15, fichier);
  for (i=0;i<128;i++) {
    fread(line,1,3*128,fichier);
    for (j=0;j<128;j++) {
	  tab_r[128 + 256*i+j] = line[3*j    ];
	  tab_g[128 + 256*i+j] = line[3*j + 1];
	  tab_b[128 + 256*i+j] = line[3*j + 2];
    }
	memcpy(tab_r + 256*128 + i*256, tab_r + 128 + i*256, 128);
	memcpy(tab_g + 256*128 + i*256, tab_g + 128 + i*256, 128);
	memcpy(tab_b + 256*128 + i*256, tab_b + 128 + i*256, 128);
  }
  fclose(fichier);



  /* tables sin/cos */
  for (i=0;i<360;i++) {
    radian = 2*i*M_PI/360;
    h = 2 + cos (radian);      
    h_cos[i] = 256 * ( h * cos (radian) );
    h_sin[i] = 256 * ( h * sin (radian) );
  }
}

int main(int argc, char **argv)
{
    int w, h, i;
    char buf[1024];

    if (argc != 2) {
        printf("usage: %s directory/\n"
               "generate a test video stream\n", argv[0]);
        exit(1);
    }

    w = DEFAULT_WIDTH;
    h = DEFAULT_HEIGHT;

    rgb_tab = malloc(w * h * 3);
    wrap = w * 3;
    width = w;
    height = h;

    init_demo();

    for(i=0;i<DEFAULT_NB_PICT;i++) {
        snprintf(buf, sizeof(buf), "%s%03d.pgm", argv[1], i);
        gen_image(i, w, h);
        pgmyuv_save(buf, w, h, rgb_tab);
    }
    
    free(rgb_tab);
    return 0;
}
