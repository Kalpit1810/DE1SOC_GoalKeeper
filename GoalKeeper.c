#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

/* This files provides address values that exist in the system */

#define SDRAM_BASE 0xC0000000
#define FPGA_ONCHIP_BASE 0xC8000000
#define FPGA_CHAR_BASE 0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE 0xFF200000
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050
#define MPCORE_PRIV_TIMER 0xFFFEC600
#define PIXEL_BUF_CTRL_BASE 0xFF203020
#define CHAR_BUF_CTRL_BASE 0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define ICE 0x8FFF
#define MAGENTA 0xF81F
#define GREY 0xB618
#define PINK 0xFC18
#define ORANGE 0xFC00

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 6
#define SAVER_WIDTH 12
#define SAVER_HEIGHT 45

#define FALSE 0
#define TRUE 1

int max(int a, int b);
int min(int a, int b);
void wait_for_vsync();
void clear_screen();
void draw_ball(int x, int y, short int colour);
void plot_pixel(int x, int y, short int line_color);
void swap(int *p1, int *p2);
void draw_feild();
void draw_saver(int saver_left_y, int saver_right_y, short int colour);
void saver_hit(int saver_left_y, int saver_right_x, int *ball_x, int *ball_y, int *dx, int *dy);
void writeText(char textPtr[], int x, int y);
bool goal_hit(int ball_x, int ball_y);
void display_score();
void wait(int t);

// global variables
volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
volatile int *key_ptr = (int *)KEY_BASE;
volatile int *sw_ptr = (int *)SW_BASE;
volatile int *timer_ptr = (int *)MPCORE_PRIV_TIMER;
volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
volatile int *HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;

const int saver_left_x = 60;
const int saver_right_x = 255;

volatile int pixel_buffer_start;
int speed = 2;
int key = 0;
int score_left = 0;
int score_right = 0;

char seg7[] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};
int main(void)
{

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen();                             // pixel_buffer_start points to the pixel buffer

    int ball_x, ball_y;
    int prev_x = 0, prev_y = 0;
    int dx, dy;
    int saver_left_y = 100;
    int saver_right_y = 100;
    int prev_saver_left = 100;
    int prev_saver_right = 100;
    int saver_left_up, saver_left_down, saver_right_up, saver_right_down;
    int goal = 0;

    // initializations
    ball_x = 320 / 2;
    ball_y = 240 / 2;
    dx = ((rand() % 2) * 2 - 1);
    dy = ((rand() % 2) * 2 - 1);

    char clear[] = "                     \0";
    writeText(clear, 37, 29);

    while (1)
    {
        // setting speed of the ball
        speed = *sw_ptr;
        speed += 5;
        if (speed > 10)
            speed = 10;

        // getting the movements of the savers
        key = *key_ptr;
        saver_right_up = key & 1;   // key 1
        saver_right_down = key & 2; // key 2
        saver_left_up = key & 4;    // key 3
        saver_left_down = key & 8;  // key 4
        while (key)
        {
            key = *key_ptr;
        }

        // display the empty feild
        draw_ball(prev_x, prev_y, 0);
        draw_feild();
        draw_saver(prev_saver_left, prev_saver_right, 0);

        // code for updating the locations of ball
        if (ball_x <= speed - 1)
            dx = 1;
        if (ball_x >= 320 - speed)
            dx = -1;

        if (ball_y <= speed)
            dy = 1;
        if (ball_y >= 240 - speed)
            dy = -1;

        prev_x = ball_x;
        prev_y = ball_y;
        ball_x += dx * speed;
        ball_y += dy * speed;

        // code for updating the locations of savers
        prev_saver_left = saver_left_y;
        prev_saver_right = saver_right_y;
        saver_left_y += saver_left_up * 20 - saver_left_down * 20;
        saver_right_y += saver_right_up * 20 - saver_right_down * 20;

        if (saver_left_y < 0)
            saver_left_y = 0;
        if (saver_right_y < 0)
            saver_right_y = 0;

        if (saver_left_y > 239 - SAVER_HEIGHT)
            saver_left_y = 239 - SAVER_HEIGHT;
        if (saver_right_y > 239 - SAVER_HEIGHT)
            saver_right_y = 239 - SAVER_HEIGHT;

        // check for saver hits
        saver_hit(saver_left_y, saver_right_y, &ball_x, &ball_y, &dx, &dy);

        // check for goal hit
        goal = goal_hit(ball_x, ball_y);
        if (goal)
        {
            clear_screen();
            char declare_goal[] = "GOAL!!!\0";
            writeText(declare_goal, 37, 29);
            wait(50);

            char clear[] = "                     \0";
            writeText(clear, 37, 29);

            // again initialize the ball
            ball_x = 320 / 2;
            ball_y = 240 / 2;
            dx = ((rand() % 2) * 2 - 1);
            dy = ((rand() % 2) * 2 - 1);
        }
        goal = 0;

        display_score();

        // code for drawing the balls and savers
        draw_saver(saver_left_y, saver_right_y, RED);
        draw_ball(ball_x, ball_y, YELLOW);

        wait_for_vsync();                           // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

int max(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}

int min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

void wait(int t)
{
    int status;
    *timer_ptr = t * 10000000; // timeout = 1/(200 MHz) x 1000x10^6 = t/10 sec
    *(timer_ptr + 2) = 0b001;  // set bits: mode = 0 (auto), enable = 1
    do
    {
        status = *(timer_ptr + 3); // read timer status
    } while (status == 0);
    *(timer_ptr + 3) = status; // reset timer flag bit
}

void wait_for_vsync()
{
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    *pixel_ctrl_ptr = 1;
    while (*(pixel_ctrl_ptr + 3) & 1)
        ;
    return;
}

void clear_screen()
{
    for (int x = 0; x < 320; x++)
        for (int y = 0; y < 240; y++)
            plot_pixel(x, y, 0);
}

void draw_ball(int x, int y, short int colour)
{
    for (int i = x; i < x + BOX_LEN; i++)
    {
        for (int j = y; j < y + BOX_LEN; j++)
        {
            plot_pixel(i, j, colour);
        }
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void draw_feild()
{
    // draw mid line
    for (int x = 159; x < 161; x++)
    {
        for (int y = 0; y < 240; y++)
        {
            plot_pixel(x, y, 0xffff);
        }
    }
    // draw left goal area
    for (int x = 29; x < 31; x++)
    {
        for (int y = 80; y < 160; y++)
        {
            plot_pixel(x, y, 0xffff);
        }
    }
    for (int x = 0; x < 31; x++)
    {
        plot_pixel(x, 80, 0xffff);
        plot_pixel(x, 81, 0xffff);
    }
    for (int x = 0; x < 31; x++)
    {
        plot_pixel(x, 158, 0xffff);
        plot_pixel(x, 159, 0xffff);
    }
    // draw right goal area
    for (int x = 290; x < 292; x++)
    {
        for (int y = 80; y < 160; y++)
        {
            plot_pixel(x, y, 0xffff);
        }
    }
    for (int x = 319; x > 292; x--)
    {
        plot_pixel(x, 80, 0xffff);
        plot_pixel(x, 81, 0xffff);
    }
    for (int x = 319; x > 292; x--)
    {
        plot_pixel(x, 158, 0xffff);
        plot_pixel(x, 159, 0xffff);
    }

    // draw mid circle
    for (int x = 120; x < 201; x++)
    {
        plot_pixel(x, 120 + sqrt(320 * x - x * x - 24000), 0xffff);
        plot_pixel(x, 120 - sqrt(320 * x - x * x - 24000), 0xffff);
        plot_pixel(x + 1, 120 + sqrt(320 * x - x * x - 24000) + 1, 0xffff);
        plot_pixel(x + 1, 120 - sqrt(320 * x - x * x - 24000) + 1, 0xffff);
    }
    for (int y = 80; y < 160; y++)
    {
        plot_pixel(160 + sqrt(240 * y - y * y - 12800), y, 0xffff);
        plot_pixel(160 - sqrt(240 * y - y * y - 12800), y, 0xffff);
        plot_pixel(160 + sqrt(240 * y - y * y - 12800) + 1, y + 1, 0xffff);
        plot_pixel(160 - sqrt(240 * y - y * y - 12800) + 1, y + 1, 0xffff);
    }
}

void draw_saver(int saver_left_y, int saver_right_y, short int colour)
{
    for (int x = saver_left_x; x < saver_left_x + SAVER_WIDTH; x++)
    {
        for (int y = saver_left_y; y < saver_left_y + SAVER_HEIGHT && y < 240; y++)
        {
            plot_pixel(x, y, colour);
        }
    }

    for (int x = saver_right_x; x < saver_right_x + SAVER_WIDTH; x++)
    {
        for (int y = saver_right_y; y < saver_right_y + SAVER_HEIGHT && y < 240; y++)
        {
            plot_pixel(x, y, colour);
        }
    }
}

void saver_hit(int saver_left_y, int saver_right_y, int *ball_x, int *ball_y, int *dx, int *dy)
{
    // left daver hit
    if (max(0, min(*ball_x + BOX_LEN, saver_left_x + SAVER_WIDTH) - max(*ball_x, saver_left_x)) && max(0, min(*ball_y + BOX_LEN, saver_left_y + SAVER_HEIGHT) - max(*ball_y, saver_left_y)))
    {
        if (*dx > 0)
            *ball_x = saver_left_x - 1;
        else
            *ball_x = (saver_left_x + SAVER_WIDTH) + 1;

        (*dx) *= -1;
    }

    // right saver hit
    if (max(0, min(*ball_x + BOX_LEN, saver_right_x + SAVER_WIDTH) - max(*ball_x, saver_right_x)) && max(0, min(*ball_y + BOX_LEN, saver_right_y + SAVER_HEIGHT) - max(*ball_y, saver_right_y)))
    {
        if (*dx < 0)
            *ball_x = (saver_right_x + SAVER_WIDTH) + 1;
        else
            *ball_x = saver_right_x - 1;

        (*dx) *= -1;
    }
}

void writeText(char textPtr[], int x, int y)
{
    volatile char *characterBuffer = (char *)0xC9000000;

    int offset = (y << 7) + x;

    while (*textPtr)
    {
        *(characterBuffer + offset) = *textPtr;
        ++textPtr;
        ++offset;
    }
}

void display_score()
{
    *HEX3_HEX0_ptr = seg7[score_right % 10];
    *HEX3_HEX0_ptr |= seg7[score_right / 10] << 8;

    *HEX5_HEX4_ptr = seg7[score_left % 10];
    *HEX5_HEX4_ptr |= seg7[score_left / 10] << 8;
}

bool goal_hit(int ball_x, int ball_y)
{
    // left goal hit
    if (max(0, min(ball_x + BOX_LEN, 25) - max(ball_x, 0)) && max(0, min(ball_y + BOX_LEN, 155) - max(ball_y, 85)))
    {
        ++score_left;
        return true;
    }

    // right goal hit
    if (max(0, min(ball_x + BOX_LEN, 319) - max(ball_x, 295)) && max(0, min(ball_y + BOX_LEN, 155) - max(ball_y, 85)))
    {
        ++score_right;
        return true;
    }

    return false;
}