// two configurations
// c1 is config if you have nothing on pin 13
// c2 will be used if you ground pin 13, through a 10k resistor
// c1 = svga, quality 10, 30 minutes
// c2 = uxga, quality 10, 30 minutes

int c1_framesize = 7;                //  10 UXGA, 9 SXGA, 7 SVGA, 6 VGA, 5 CIF
int c1_quality = 3;                 //  quality on the 1..63 scale  - lower is better quality and bigger files - must be higher than the jpeg_quality in camera_config
int c1_avi_length = 3600;            // how long a movie in seconds -- 1800 sec = 30 min

int c2_framesize = 7;
int c2_quality = 3;
int c2_avi_length = 3600;

int c1_or_c2 = 1;
int framesize = c1_framesize;
int quality = c1_quality;
int avi_length = c1_avi_length;


int MagicNumber = 10;                // change this number to reset the eprom in your esp32 for file numbers 

static const char devname[] = "file";

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float most_recent_fps = 0;
int most_recent_avg_framesize = 0;

uint8_t* framebuffer;
int framebuffer_len;

uint8_t framebuffer_static[64 * 1024 + 20];
int framebuffer_len_static;


#include "esp_camera.h"

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

camera_fb_t * fb_curr = NULL;
camera_fb_t * fb_next = NULL;

#include <stdio.h>
static esp_err_t cam_err;

int first = 1;
//int frame_cnt = 0;
long frame_start = 0;
long frame_end = 0;
long frame_total = 0;
long frame_average = 0;
long loop_average = 0;
long loop_total = 0;
long total_frame_data = 0;
long last_frame_length = 0;
int done = 0;
long avi_start_time = 0;
long avi_end_time = 0;
int stop = 0;
int stop_2nd_opinion = -2;
int stop_1st_opinion = -1;

int we_are_already_stopped = 0;
long total_delay = 0;
long bytes_before_last_100_frames = 0;
long time_before_last_100_frames = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  Avi Writer Stuff here

FILE *avifile = NULL;
FILE *idxfile = NULL;

long bp;
long ap;
long bw;
long aw;

int diskspeed = 0;
char fname[100];

static int i = 0;
uint8_t temp = 0, temp_last = 0;
unsigned long fileposition = 0;
uint16_t frame_cnt = 0;
uint16_t remnant = 0;
uint16_t frame_max=18000; //max frames
uint32_t length = 0;
uint32_t startms;
uint32_t elapsedms;
uint32_t uVideoLen = 0;
bool is_header = false;
int bad_jpg = 0;
int extend_jpg = 0;
int normal_jpg = 0;

int file_number = 0;
int file_group = 0;
long boot_time = 0;

long totalp;
long totalw;
float avgp;
float avgw;

#define BUFFSIZE 512

uint8_t buf[BUFFSIZE];

#define AVIOFFSET 240 // AVI main header length

unsigned long movi_size = 0;
unsigned long jpeg_size = 0;
unsigned long idx_offset = 0;

uint8_t zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t dc_buf[4] = {0x30, 0x30, 0x64, 0x63};    // "00dc"
uint8_t dc_and_zero_buf[8] = {0x30, 0x30, 0x64, 0x63, 0x00, 0x00, 0x00, 0x00};

uint8_t avi1_buf[4] = {0x41, 0x56, 0x49, 0x31};    // "AVI1"
uint8_t idx1_buf[4] = {0x69, 0x64, 0x78, 0x31};    // "idx1"

uint8_t  vga_w[2] = {0x80, 0x02}; // 640
uint8_t  vga_h[2] = {0xE0, 0x01}; // 480
uint8_t  cif_w[2] = {0x90, 0x01}; // 400
uint8_t  cif_h[2] = {0x28, 0x01}; // 296
uint8_t svga_w[2] = {0x20, 0x03}; // 800
uint8_t svga_h[2] = {0x58, 0x02}; // 600
uint8_t sxga_w[2] = {0x00, 0x05}; // 1280
uint8_t sxga_h[2] = {0x00, 0x04}; // 1024
uint8_t uxga_w[2] = {0x40, 0x06}; // 1600
uint8_t uxga_h[2] = {0xB0, 0x04}; // 1200


const int avi_header[AVIOFFSET] PROGMEM = {
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4E, 0x46, 0x4F,
  0x10, 0x00, 0x00, 0x00, 0x6A, 0x61, 0x6D, 0x65, 0x73, 0x7A, 0x61, 0x68, 0x61, 0x72, 0x79, 0x20,
  0x76, 0x31, 0x30, 0x20, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};


//
// Writes an uint32_t in Big Endian at current file position
//
static void inline print_quartet(unsigned long i, FILE * fd)
{
  uint8_t y[4];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  size_t i1_err = fwrite(y , 1, 4, fd);
}

//
// Writes 2 uint32_t in Big Endian at current file position
//
static void inline print_2quartet(unsigned long i, unsigned long j, FILE * fd)
{
  uint8_t y[8];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  y[4] = j % 0x100;
  y[5] = (j >> 8) % 0x100;
  y[6] = (j >> 16) % 0x100;
  y[7] = (j >> 24) % 0x100;
  size_t i1_err = fwrite(y , 1, 8, fd);
}

//
// if we have no camera, or sd card, then flash rear led on and off to warn the human SOS - SOS
//
void major_fail() {

  Serial.println(" ");

  for  (int i = 0;  i < 10; i++) {                 // 10 loops or about 100 seconds then reboot
    for (int j = 0; j < 3; j++) {
      digitalWrite(33, LOW);   delay(150);
      digitalWrite(33, HIGH);  delay(150);
    }
    delay(1000);

    for (int j = 0; j < 3; j++) {
      digitalWrite(33, LOW);  delay(500);
      digitalWrite(33, HIGH); delay(500);
    }
    delay(1000);
    Serial.print("Major Fail  "); Serial.print(i); Serial.print(" / "); Serial.println(10);
  }

//  ESP.restart();
}


  
#include "SD_MMC.h"                         // sd card
 #include <SPI.h>
 #include <FS.h>                             // gives file access
 #define SD_CS 5  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  eprom functions  - increment the file_group, so files are always unique
//

#include <EEPROM.h>

struct eprom_data {
  int eprom_good;
  int file_group;
};

void do_eprom_read() {

  eprom_data ed;

  EEPROM.begin(200);
  EEPROM.get(0, ed);

  if (ed.eprom_good == MagicNumber) {
    Serial.println("Good settings in the EPROM ");
    file_group = ed.file_group;
    file_group++;
    Serial.print("New File Group "); Serial.println(file_group );
  } else {
    Serial.println("No settings in EPROM - Starting with File Group 1 ");
    file_group = 1;
  }
  do_eprom_write();
  file_number = 1;
}

void do_eprom_write() {

  eprom_data ed;

  ed.eprom_good = MagicNumber;
  ed.file_group  = file_group;

  Serial.println("Writing to EPROM ...");

  EEPROM.begin(200);
  EEPROM.put(0, ed);
  EEPROM.commit();
  EEPROM.end();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Make the avi functions
//
//   start_avi() - open the file and write headers
//   another_pic_avi() - write one more frame of movie
//   end_avi() - write the final parameters and close the file


//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// start_avi - open the files and write in headers
//

void start_avi() {

  Serial.println("Starting an avi ");

  sprintf(fname, "/sdcard/%s_%d_%03d.avi",  devname, file_group, file_number);
  
  file_number++;

  avifile = fopen(fname, "w");
  idxfile = fopen("/sdcard/idx.tmp", "w");

  if (avifile != NULL)  {
    Serial.printf("File open: %s\n", fname);
  }  else  {
    Serial.println("Could not open file");
    major_fail();
  }

  if (idxfile != NULL)  {
    Serial.printf("File open: %s\n", "/sdcard/idx.tmp");
  }  else  {
    Serial.println("Could not open file");
    major_fail();
  }

  for ( i = 0; i < AVIOFFSET; i++)
  {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }

  size_t err = fwrite(buf, 1, AVIOFFSET, avifile);

  if (framesize == 6) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(vga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(vga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(vga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(vga_h, 1, 2, avifile);

  } else if (framesize == 10) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(uxga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(uxga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(uxga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(uxga_h, 1, 2, avifile);

  } else if (framesize == 9) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(sxga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(sxga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(sxga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(sxga_h, 1, 2, avifile);

  } else if (framesize == 7) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(svga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(svga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(svga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(svga_h, 1, 2, avifile);

  }  else if (framesize == 5) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(cif_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(cif_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(cif_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(cif_h, 1, 2, avifile);
  }

  fseek(avifile, AVIOFFSET, SEEK_SET);

  Serial.print("\nRecording ");


  startms = millis();

  totalp = 0;
  totalw = 0;

  jpeg_size = 0;
  movi_size = 0;
  uVideoLen = 0;
  idx_offset = 4;

  //frame_cnt = 0;

  bad_jpg = 0;
  extend_jpg = 0;
  normal_jpg = 0;

} // end of start avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  another_save_avi saves another frame to the avi file, uodates index
//           -- pass in a fb pointer to the frame to add
//

void another_save_avi(camera_fb_t * fb ) {

  int fblen;
  fblen = fb->len;

  int fb_block_length;
  uint8_t* fb_block_start;

  jpeg_size = fblen;
  movi_size += jpeg_size;
  uVideoLen += jpeg_size;

  remnant = (4 - (jpeg_size & 0x00000003)) & 0x00000003;

  bw = millis();
  long frame_write_start = millis();

  framebuffer_static[3] = 0x63;
  framebuffer_static[2] = 0x64;
  framebuffer_static[1] = 0x30;
  framebuffer_static[0] = 0x30;

  int jpeg_size_rem = jpeg_size + remnant;

  framebuffer_static[4] = jpeg_size_rem % 0x100;
  framebuffer_static[5] = (jpeg_size_rem >> 8) % 0x100;
  framebuffer_static[6] = (jpeg_size_rem >> 16) % 0x100;
  framebuffer_static[7] = (jpeg_size_rem >> 24) % 0x100;

  fb_block_start = fb->buf;

  if (fblen > 64 * 1024 - 8 ) {
    fb_block_length = 64 * 1024;
    fblen = fblen - (64 * 1024 - 8);
    memcpy(framebuffer_static + 8, fb_block_start, fb_block_length - 8);
    fb_block_start = fb_block_start + fb_block_length - 8;

  } else {
    fb_block_length = fblen + 8  + remnant;
    memcpy(framebuffer_static + 8, fb_block_start,  fblen);
    fblen = 0;
  }

  size_t err = fwrite(framebuffer_static, 1, fb_block_length, avifile);
  if (err != fb_block_length) {
    Serial.print("Error on avi write: err = "); Serial.print(err);
    Serial.print(" len = "); Serial.println(fb_block_length);
  }

  while (fblen > 0) {

    if (fblen > 64 * 1024) {
      fb_block_length = 64 * 1024;
      fblen = fblen - fb_block_length;
    } else {
      fb_block_length = fblen  + remnant;
      fblen = 0;
    }

    memcpy(framebuffer_static, fb_block_start, fb_block_length);

    size_t err = fwrite(framebuffer_static, 1, fb_block_length, avifile);
    if (err != fb_block_length) {
      Serial.print("Error on avi write: err = "); Serial.print(err);
      Serial.print(" len = "); Serial.println(fb_block_length);
    }

    fb_block_start = fb_block_start + fb_block_length;
  }

  long frame_write_end = millis();

  print_2quartet(idx_offset, jpeg_size, idxfile);

  idx_offset = idx_offset + jpeg_size + remnant + 8;

  movi_size = movi_size + remnant;

  totalw = totalw + millis() - bw;

} // end of another_pic_avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  end_avi writes the index, and closes the files
//

void end_avi() {

  unsigned long current_end = 0;

  current_end = ftell (avifile);

  Serial.println("End of avi - closing the files");


  if (frame_cnt <  10 ) {
    Serial.println("Recording screwed up, less than 10 frames, forget index\n");
    fclose(idxfile);
    fclose(avifile);
    int xx = remove("/sdcard/idx.tmp");
    int yy = remove(fname);
  } else {

    elapsedms = millis() - startms;

    float fRealFPS = (1000.0f * (float)frame_cnt) / ((float)elapsedms);

    float fmicroseconds_per_frame = 1000000.0f / fRealFPS;
    uint8_t iAttainedFPS = round(fRealFPS);
    uint32_t us_per_frame = round(fmicroseconds_per_frame);

    //Modify the MJPEG header from the beginning of the file, overwriting various placeholders

    fseek(avifile, 4 , SEEK_SET);
    print_quartet(movi_size + 240 + 16 * frame_cnt + 8 * frame_cnt, avifile);

    fseek(avifile, 0x20 , SEEK_SET);
    print_quartet(us_per_frame, avifile);

    unsigned long max_bytes_per_sec = movi_size * iAttainedFPS / frame_cnt;

    fseek(avifile, 0x24 , SEEK_SET);
    print_quartet(max_bytes_per_sec, avifile);

    fseek(avifile, 0x30 , SEEK_SET);
    print_quartet(frame_cnt, avifile);

    fseek(avifile, 0x8c , SEEK_SET);
    print_quartet(frame_cnt, avifile);

    fseek(avifile, 0x84 , SEEK_SET);
    print_quartet((int)iAttainedFPS, avifile);

    fseek(avifile, 0xe8 , SEEK_SET);
    print_quartet(movi_size + frame_cnt * 8 + 4, avifile);

    Serial.println(F("\n** Video recorded and saved **\n"));
    Serial.print(F("Recorded "));
    Serial.print(elapsedms / 1000);
    Serial.print(F("s in "));
    Serial.print(frame_cnt);
    Serial.print(F(" frames\nFile size is "));
    Serial.print(movi_size + 12 * frame_cnt + 4);
    Serial.print(F(" bytes\nActual FPS is "));
    Serial.print(fRealFPS, 2);
    Serial.print(F("\nMax data rate is "));
    Serial.print(max_bytes_per_sec);
    Serial.print(F(" byte/s\nFrame duration is "));  Serial.print(us_per_frame);  Serial.println(F(" us"));
    Serial.print(F("Average frame length is "));  Serial.print(uVideoLen / frame_cnt);  Serial.println(F(" bytes"));
    Serial.print("Average picture time (ms) "); Serial.println( 1.0 * totalp / frame_cnt);
    Serial.print("Average write time (ms)   "); Serial.println( 1.0 * totalw / frame_cnt );
    Serial.print("Normal jpg % ");  Serial.println( 100.0 * normal_jpg / frame_cnt, 1 );
    Serial.print("Extend jpg % ");  Serial.println( 100.0 * extend_jpg / frame_cnt, 1 );
    Serial.print("Bad    jpg % ");  Serial.println( 100.0 * bad_jpg / frame_cnt, 5 );


    Serial.printf("Writng the index, %d frames\n", frame_cnt);
    fseek(avifile, current_end, SEEK_SET);

    fclose(idxfile);

    size_t i1_err = fwrite(idx1_buf, 1, 4, avifile);

    print_quartet(frame_cnt * 16, avifile);

    idxfile = fopen("/sdcard/idx.tmp", "r");

    if (idxfile != NULL)  {
      Serial.printf("File open: %s\n", "/sdcard/idx.tmp");
    }  else  {
      Serial.println("Could not open index file");
      major_fail();
    }

    char * AteBytes;
    AteBytes = (char*) malloc (8);

    for (int i = 0; i < frame_cnt; i++) {
      size_t res = fread ( AteBytes, 1, 8, idxfile);
      size_t i1_err = fwrite(dc_buf, 1, 4, avifile);
      size_t i2_err = fwrite(zero_buf, 1, 4, avifile);
      size_t i3_err = fwrite(AteBytes, 1, 8, avifile);
    }

    free(AteBytes);

    fclose(idxfile);
    fclose(avifile);
    int xx = remove("/sdcard/idx.tmp");
  }
  Serial.println("---");

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wifi Credentials
//
const char* ssid = "Enter-Your-SSID";
const char* password = "Enter-SSID-Password";


//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Web Page
//
#include <WiFi.h>
#include <WebServer.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"

#define PART_BOUNDARY "123456789000000000000987654321"
#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=" PART_BOUNDARY
#define STREAM_BOUNDARY "\r\n--" PART_BOUNDARY "\r\n"
#define STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

WiFiClient streamClient;

TaskHandle_t streamTaskHandle = NULL;

WebServer server(80);

bool isRecording = false;
bool startRecording = false;
bool stopRecording = false;

void handleRoot() {
  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Camera Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #1a1a1a; color: #ffffff; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; }";
  html += ".container { max-width: 800px; width: 100%; }";
  html += ".stream-container { margin-bottom: 20px;  text-align: center;}";
  html += ".controls { display: flex; justify-content: center; gap: 10px; margin-bottom: 20px; }";
  html += "button { padding: 10px 20px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
  html += ".start { background-color: #4CAF50; color: white; }";
  html += ".stop { background-color: #D22B2B; color: white; }";
  html += "button:hover { opacity: 0.8; }";
  html += "#status { text-align: center; margin-bottom: 20px; }";
  html += "table { width: 100%; border-collapse: collapse; margin-top: 20px; }";
  html += "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #2c2c2c; }";
  html += "th { background-color: #2c2c2c; color: #ffffff; }";
  html += "tr:nth-child(even) { background-color: #222222; }";
  html += "a { color: #4CAF50; text-decoration: none; margin-right: 10px; }";
  html += "a:hover { text-decoration: underline; }";
  html += ".overlay { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.5); z-index: 1000; justify-content: center; align-items: center; color: white; font-size: 24px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<div class='stream-container'>";
  html += "<img src='/stream' style='width:100%; max-width:640px; height:auto; border: 3px solid #3b3b3b;' />";
  html += "</div>";
  html += "<div class='controls'>";
  html += "<button class='start' onclick='sendCommand(\"start\")'>Start Recording</button>";
  html += "<button class='stop' onclick='sendCommand(\"stop\")'>Stop Recording</button>";
  html += "</div>";
  html += "<div id='status'></div>";
  html += "<br><br>";
  html += "<table>";
  html += "<tr><th>File Name</th><th>Size</th><th>Actions</th></tr>";

  File root = SD_MMC.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        html += "<tr><td>" + String(file.name()) + "</td><td>" + formatBytes(file.size()) + "</td><td>";
        html += "<a href='/download?file=" + String(file.name()) + "'>Download</a> ";
        html += "<a href='#' onclick='deleteFile(\"" + String(file.name()) + "\"); return false;'>Delete</a>";
        html += "</td></tr>";
      }
      file = root.openNextFile();
    }
    root.close();
  }

  html += "</table>";
  html += "</div>";
  html += "<div id='overlay' class='overlay'></div>";
  html += "<script>";
  html += "function sendCommand(action) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (this.readyState == 4 && this.status == 200) {";
  html += "      document.getElementById('status').innerHTML = this.responseText;";
  html += "      if (action === 'stop') {";
  html += "        document.getElementById('overlay').style.display = 'flex';";
  html += "        document.getElementById('overlay').innerHTML = 'Stopping recording... Page will reload shortly.';";
  html += "        setTimeout(function() {";
  html += "          location.reload();";
  html += "        }, 3000);";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhr.open('GET', '/' + action, true);";
  html += "  xhr.send();";
  html += "}";
  html += "function deleteFile(filename) {";
  html += "  if (confirm('Are you sure you want to delete this file?')) {";
  html += "    var xhr = new XMLHttpRequest();";
  html += "    xhr.onreadystatechange = function() {";
  html += "      if (this.readyState == 4) {";
  html += "        if (this.status == 200 || this.status == 303) {";
  html += "          document.getElementById('overlay').style.display = 'flex';";
  html += "          document.getElementById('overlay').innerHTML = 'File deleted. Reloading page...';";
  html += "          setTimeout(function() {";
  html += "            location.reload();";
  html += "          }, 2000);";
  html += "        } else {";
  html += "          alert('Delete failed');";
  html += "        }";
  html += "      }";
  html += "    };";
  html += "    xhr.open('GET', '/delete?file=' + encodeURIComponent(filename), true);";
  html += "    xhr.send();";
  html += "  }";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}


void handleStart() {
  startRecording = true;
  server.send(200, "text/plain", "Starting recording...");
}

void handleStop() {
  stopRecording = true;
  server.send(200, "text/plain", "Stopping recording... Please wait.");
}

void handleStatus() {
  String status = isRecording ? "Recording in progress" : "Not recording";
  server.send(200, "text/plain", status);
}

void handleStream() {
  streamClient = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: " STREAM_CONTENT_TYPE "\r\n\r\n";
  streamClient.print(response);

  if (streamTaskHandle == NULL) {
    xTaskCreatePinnedToCore(
      streamTask,       //funtion name
      "stream_task",    //task name
      8192,             //stack size
      NULL,             //task parameters
      1,                //priority
      &streamTaskHandle,//task handle
      0                 //core
    );
  }
}

void streamTask(void * parameter) {
  while (true) {
    if (streamClient.connected()) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        streamClient.print(STREAM_BOUNDARY);
        char part_buf[64];
        snprintf(part_buf, 64, STREAM_PART, fb->len);
        streamClient.print(part_buf);
        streamClient.write(fb->buf, fb->len);
        esp_camera_fb_return(fb);
      }
    } else {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void handleFileDownload() {
  String filename = server.arg("file");
  if (filename == "") {
    server.send(400, "text/plain", "File name is required");
    return;
  }

  File file = SD_MMC.open("/" + filename, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void handleFileDelete() {
  String filename = server.arg("file");
  if (filename == "") {
    server.send(400, "text/plain", "File name is required");
    return;
  }

  if (SD_MMC.remove("/" + filename)) {
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(500, "text/plain", "Delete failed");
  }
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0, 2) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String((bytes / 1024.0) / 1024.0, 2) + " MB";
  else return String(((bytes / 1024.0) / 1024.0) / 1024.0, 2) + " GB";
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// WebDAV
//
/*
#include <ESPWebDAV.h>

WiFiServer tcp(81);
ESPWebDAV dav;
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n---");
  pinMode(4, OUTPUT);  

  Serial.print("setup, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

   // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Setting up the camera ...");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 5;
  config.fb_count = 1;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    while (1);
  }

  SD_MMC.begin("/sdcard", true);
  // Check if the card is present and can be initialized:
  if (!SD_MMC.begin("/sdcard", false)) {
    Serial.println("Card failed, or not present");
    while (1);
  }

  /*
  tcp.begin();
  dav.begin(&tcp, &SD_MMC);
  dav.setTransferStatusCallback([](const char* name, int percent, bool receive){
    Serial.printf("%s: '%s': %d%%\n", receive ? "recv" : "send", name, percent);
  });
  
  Serial.println("WebDAV server started");
  */
  
 
  Serial.println("Warming up the camera ... here are some frames sizes ...");

  for (int i = 0; i < 4; i++) {
    camera_fb_t * fb = esp_camera_fb_get();
    Serial.printf("frame %d, len %d\n", i, fb->len);
    esp_camera_fb_return(fb);
    delay(100);
  }

  do_eprom_read();

  framebuffer = (uint8_t*)ps_malloc(512 * 1024); // buffer to store a jpg in motion

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.on("/stream", handleStream);
  server.on("/download", HTTP_GET, handleFileDownload);
  server.on("/delete", HTTP_GET, handleFileDelete);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("  End of setup()\n\n");

  Serial.print("For Camera Control Go to: http://");
  Serial.print(WiFi.localIP());
  Serial.print("\n\n");

  /*
  Serial.print("WebDAV: http://");
  Serial.print(WiFi.localIP());
  Serial.print(":81/");
  Serial.print("\n\n");
  */

  boot_time = millis();
}

void loop() {
  server.handleClient();
  
  /*
  dav.handleClient();
  */
  
  if (!streamClient.connected() && streamTaskHandle != NULL) {
    vTaskDelete(streamTaskHandle);
    streamTaskHandle = NULL;
  }
  
  if (startRecording && !isRecording) {
    isRecording = true;
    startRecording = false;
    frame_cnt = 0;
    digitalWrite(4, HIGH);
    Serial.println("Starting recording...");
    
    framesize = c2_framesize;
    quality = c2_quality;
    avi_length = c2_avi_length;
    
    sensor_t * ss = esp_camera_sensor_get();
    ss->set_quality(ss, quality);
    ss->set_framesize(ss, (framesize_t) framesize);
    ss->set_brightness(ss, 1);  //up the brightness just a bit
    ss->set_saturation(ss, -2); //lower the saturation
    
    avi_start_time = millis();
    Serial.printf("Start the avi ... at %d\n", avi_start_time);
    Serial.printf("Framesize %d, quality %d, length %d seconds\n\n", framesize, quality, avi_length);
    fb_curr = esp_camera_fb_get();
    start_avi();
    fb_next = esp_camera_fb_get();                  
    another_save_avi(fb_curr);                  // put first frame in avi
    esp_camera_fb_return(fb_curr);               // get rid of first frame
    fb_curr = NULL;
  }

  if (isRecording) {
    frame_cnt++;
    if (frame_cnt > 1) {
      fb_curr = esp_camera_fb_get();    // should take near zero, unless the sd is faster than the camera, when we will have to wait for the camera
      another_save_avi(fb_curr);
      esp_camera_fb_return(fb_curr);
      fb_curr = NULL;
    }
  }

  if (stopRecording && isRecording) {
    digitalWrite(4, LOW);
    isRecording = false;
    stopRecording = false;
    Serial.println("Stopping recording...");
    
    end_avi();

    avi_end_time = millis();
    float fps = frame_cnt / ((avi_end_time - avi_start_time) / 1000);
    Serial.printf("End the avi at %d. It was %d frames, %d ms at %.1f fps...\n", millis(), frame_cnt, avi_end_time - avi_start_time, fps);
  }
}
