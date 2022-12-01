extern int sexmachine_debug;

int getSerialData(int verbose);
int setSerialGun(int num);
long MAP(long x, long in_min, long in_max, long out_min, long out_max);

extern char serial_buffer[100];

struct pi_timings {
  int h_active_pixels;
  int h_sync_polarity;
  int h_front_porch;
  int h_sync_pulse;
  int h_back_porch;
  int v_active_lines;
  int v_sync_polarity;
  int v_front_porch;
  int v_sync_pulse;
  int v_back_porch;
  int v_sync_offset_a;
  int v_sync_offset_b;
  int pixel_rep;
  int frame_rate;
  int interlaced;
  long pixel_freq;
  int aspect_ratio;
};

extern int gunTriggered;
extern int gunX;
extern int gunY;
extern int gunShot;
extern struct pi_timings hdmi_timings;
extern int xres;
extern int yres;
extern int game_xres;
extern int game_yres;
extern char game_name[50];
extern int game_max_x;
extern int game_max_y;
extern int game_min_x;
extern int game_min_y;
extern int tune_x;
extern int tune_y;

void sexmachine_minmax();
