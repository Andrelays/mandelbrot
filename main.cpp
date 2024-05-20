#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <emmintrin.h>
#include <x86intrin.h>
#include <unistd.h>
#include "libraries/utilities/myassert.h"

const char* font_path = "/usr/share/fonts/truetype/firacode/FiraCode-Bold.ttf";

const int SCREEN_WIDTH      = 1000;
const int SCREEN_HEIGHT     = 800;
const size_t SIZE           = 8;
const size_t N_MAX          = 100;
const float dx              = 1/400.f;
const float dy              = 1/400.f;
const float dscale          = 1.2f;
const float RADIUS          = 100;
const float X_SHIFT         = -0.55f;
const size_t SIZE_OF_BUFFER = 30;
const unsigned long long clock_speed_spu = 3000000000;

const __m256 r2max      = _mm256_set1_ps (RADIUS);
const __m256 _76543210  = _mm256_set_ps  (7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);
const __m256 nmax       = _mm256_set1_ps (N_MAX);

int number_options = 3;

void        sdl_ctor();
void        sdl_dtor();
void        calc_all_colors(SDL_Color* colors);
void        CleanBuffer(char* buff, size_t leng);

bool        DrawMandelbrot1(float* x_center, float* y_center, float* scale, SDL_Color* colors);
bool        DrawMandelbrot2(float* x_center, float* y_center, float* scale, SDL_Color* colors);
bool        DrawMandelbrot3(float* x_center, float* y_center, float* scale, SDL_Color* colors);

SDL_Window   *window = NULL;
SDL_Event    event;
SDL_Surface  *surfaceMessage = NULL;
SDL_Surface  *windowSurface  = NULL;

int main()
{
    sdl_ctor();

    float x_center = X_SHIFT;
    float y_center = 0;
    float scale = 1;
    bool run = true;
    uint64_t t1 = 0;
    uint64_t t2 = 0;
    char fps_buffer[SIZE_OF_BUFFER] = {};

    SDL_Color color_text = {255, 255, 255, 255};
    SDL_Color colors[N_MAX+1] = {};


    calc_all_colors(colors);

    while (run)
    {
        //Process Keys
        while(SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                run = false;
                break;
            }

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP)        y_center += dy*scale;
                if (event.key.keysym.sym == SDLK_DOWN)      y_center -= dy*scale;
                if (event.key.keysym.sym == SDLK_RIGHT)     x_center -= dx*scale;
                if (event.key.keysym.sym == SDLK_LEFT)      x_center += dx*scale;

                if (event.key.keysym.sym == SDLK_EQUALS)    scale /= dscale;
                if (event.key.keysym.sym == SDLK_MINUS)     scale *= dscale;

                if (event.key.keysym.sym == SDLK_1)         number_options = 1;
                if (event.key.keysym.sym == SDLK_2)         number_options = 2;
                if (event.key.keysym.sym == SDLK_3)         number_options = 3;
            }
        }

        CleanBuffer(fps_buffer, SIZE_OF_BUFFER);

        SDL_FillRect(windowSurface, NULL, 0xFF000000);
        switch (number_options)
        {
            case 1:
            if (!DrawMandelbrot1(&x_center, &y_center, &scale, colors))
            {
                run = false;
            }
            break;
            case 2:
            if (!DrawMandelbrot2(&x_center, &y_center, &scale, colors))
            {
                run = false;
            }
            break;
            case 3:
            if (!DrawMandelbrot3(&x_center, &y_center, &scale, colors))
            {
                run = false;
            }
            break;
            default: break;
        }

        t2 = __rdtsc();
        float fps_float = (float)clock_speed_spu / (float)(t2 - t1);
        t1 = __rdtsc();

        printf("\e[1;1H\e[2J\nfps = %.2f\n", fps_float);

        SDL_UpdateWindowSurface(window);
    }
    // Clean up and quit SDL
    sdl_dtor();
    return 0;
}

void sdl_ctor()
{
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    int screen_width = DM.w;
    int screen_height = DM.h;

    const int window_pos_x = (screen_width - SCREEN_WIDTH)/2;
    const int window_pos_y = (screen_height - SCREEN_HEIGHT)/2;

    window = SDL_CreateWindow( "MANDELBROT SET",
                                window_pos_x, window_pos_y,
                                SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    MYASSERT(window != NULL, SDL_ERROR, return);

    windowSurface = SDL_GetWindowSurface(window);

    return;
}

void sdl_dtor()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}

void calc_all_colors(SDL_Color* colors)
{
    for (size_t i = 0; i < (N_MAX); i++)
    {
        // Normalize the value to the range [0, 1]
        float normalized = -(float)i / (float)N_MAX;
        // Use sine waves to create a colorful gradient with smooth transitions
        float r_n = (float) sin(2 * M_PI * normalized * 4.0 + M_PI / 4);
        float g_n = (float) sin(2 * M_PI * normalized * 8.0 + M_PI / 8);
        float b_n = (float) sin(2 * M_PI * normalized * 16.0 + M_PI / 16);
        // Convert to the range [0, 255]
        colors[i].b = (Uint8) ((r_n + 1) * 127.5);
        colors[i].g = (Uint8) ((g_n + 1) * 127.5);
        colors[i].r = (Uint8) ((b_n + 1) * 127.5);
        colors[i].a = 255;
    }
}

void CleanBuffer(char* buff, size_t leng)
{
    size_t pos = 0;
    while ((pos < leng) && (buff[pos] != '\0'))
    {
        buff[pos] = '\0';
    }
}

bool DrawMandelbrot1(float* x_center, float* y_center, float* scale, SDL_Color* colors)
{
    float x_0 = 0;
    float y_0 = 0;
    float x = 0;
    float y = 0;
    float x2 = 0;
    float y2 = 0;
    float xy = 0;
    float r2 = 0;
    float r2max = RADIUS;
    bool color_flag = 0;
    for (int y_i = 0; y_i < SCREEN_HEIGHT; y_i++)
    {
        //check for close
        if (event.type == SDL_QUIT) {
            return 0;
        }
        //set line with height = y0 and x - counting
        x_0 = *x_center + (                    - (float)SCREEN_WIDTH *(*scale)/2)*dx;
        y_0 = *y_center + ((float)y_i*(*scale) - (float)SCREEN_HEIGHT*(*scale)/2)*dy;
        for (int x_i = 0; x_i < SCREEN_WIDTH; x_i++ , x_0 += dx*(*scale))
        {
            color_flag = 0;
            //counting (x,y)
            x = x_0;
            y = y_0;
            //check if (x,y) not run away from circle
            size_t n = 0;

            for (; n < N_MAX; n++)
            {
                x2 = x * x;
                y2 = y * y;
                xy = x * y;

                r2 = (x2 + y2);
                if (r2 > r2max) {
                    color_flag = 1;
                    break;
                }

                x = x2 - y2 + x_0;
                y = 2 * xy  + y_0;
            }

            if (color_flag)
            {
                ((int*)(windowSurface->pixels))[y_i * SCREEN_WIDTH + x_i] = *((int*)&colors[n]);
            }
        }
    }
    return 1;
}

bool DrawMandelbrot2(float* x_center, float* y_center, float* scale, SDL_Color* colors)
{
    float x_0 = 0;
    float y_0 = 0;
    float x[SIZE] = {};
    float y[SIZE] = {};
    float x2[SIZE] = {};
    float y2[SIZE] = {};
    float xy[SIZE] = {};
    float r2[SIZE] = {};
    float x_0_arr[SIZE] = {};
    int mask = 0;
    float r2max = RADIUS;
    bool color_flag[SIZE] = {};

    for (int y_i = 0; y_i < SCREEN_HEIGHT; y_i++)
    {
        //check for close
        if (event.type == SDL_QUIT) {
            return 0;
        }

        //set line with height = y0 and x - counting
        x_0 = *x_center + (                    - (float)SCREEN_WIDTH *(*scale)/2)*dx;
        y_0 = *y_center + ((float)y_i*(*scale) - (float)SCREEN_HEIGHT*(*scale)/2)*dy;

        for (int x_i = 0; x_i < SCREEN_WIDTH; x_i += 8 , x_0 += 8*dx*(*scale))
        {
            //counting (x,y)
            for (size_t i = 0; i < SIZE; i++) color_flag[i] = 0;
            for (size_t i = 0; i < SIZE; i++) x_0_arr[i] = x_0 + dx*((float)i)*(*scale);
            for (size_t i = 0; i < SIZE; i++) x[i] = x_0_arr[i];
            for (size_t i = 0; i < SIZE; i++) y[i] = y_0;
            //check if (x,y) not run away from circle
            size_t n[SIZE] = {};

            for (size_t m = 0; m < N_MAX; m++)
            {
                int cmp[SIZE] = {};

                for (size_t k = 0; k < SIZE; k++)    x2[k] = x[k] * x[k];
                for (size_t k = 0; k < SIZE; k++)    y2[k] = y[k] * y[k];
                for (size_t k = 0; k < SIZE; k++)    xy[k] = x[k] * y[k];
                for (size_t k = 0; k < SIZE; k++)    r2[k] = (x2[k] + y2[k]);
                for (size_t k = 0; k < SIZE; k++) if (r2[k] <= r2max) cmp[k] = 1;
                for (size_t k = 0; k < SIZE; k++) if (r2[k] > r2max)  color_flag[k] = 1;
                mask = 0;
                for (size_t k = 0; k < SIZE; k++) mask |= (cmp[k] << k);
                if (!mask) break;
                for (size_t k = 0; k < SIZE; k++) n[k] = n[k] + (size_t)cmp[k];
                for (size_t k = 0; k < SIZE; k++) x[k] = x2[k] - y2[k] + x_0_arr[k];
                for (size_t k = 0; k < SIZE; k++) y[k] = 2 * xy[k]     + y_0;
            }

            for (size_t j = 0; j < SIZE; j++)
            {
                if (color_flag[j])
                {
                    ((int*)(windowSurface->pixels))[y_i * SCREEN_WIDTH + x_i + j] = *((int*)&colors[n[j]]);
                }
            }
        }
    }
    return 1;
}

bool DrawMandelbrot3(float* x_center, float* y_center, float* scale, SDL_Color* colors)
{
    for (int y_i = 0; y_i < SCREEN_HEIGHT; y_i++)
    {
        //check for close
        if (event.type == SDL_QUIT) {
            return 0;
        }
        //set line with height = y0 and x - counting
        float x_0 = *x_center + (                    - (float)SCREEN_WIDTH *(*scale)/2)*dx;
        float y_0 = *y_center + ((float)y_i*(*scale) - (float)SCREEN_HEIGHT*(*scale)/2)*dy;
        for (int x_i = 0; x_i < SCREEN_WIDTH; x_i += 8 , x_0 += 8*dx*(*scale))
        {
            //counting (x,y)
            __m256 x_0_arr = _mm256_add_ps (_mm256_set1_ps (x_0), _mm256_mul_ps (_76543210, _mm256_set1_ps (dx*(*scale))));
            __m256 y_0_arr =                                                                _mm256_set1_ps (y_0);
            __m256 x = x_0_arr;
            __m256 y = y_0_arr;
            __m256i n = _mm256_setzero_si256();
            __m256 cmp = _mm256_setzero_ps();

            for (size_t m = 0; m < N_MAX; m++)
            {
                __m256 x2 = _mm256_mul_ps (x, x);
                __m256 y2 = _mm256_mul_ps (y, y);
                __m256 xy = _mm256_mul_ps (x, y);
                __m256 r2 = _mm256_add_ps (x2,y2);
                cmp = _mm256_cmp_ps (r2, r2max, _CMP_LE_OS);
                int mask = 0;
                mask = _mm256_movemask_ps (cmp);
                if (!mask) break;
                n = _mm256_sub_epi32 (n, _mm256_castps_si256(cmp));
                x = _mm256_add_ps (_mm256_sub_ps(x2,y2), x_0_arr);
                y = _mm256_add_ps (_mm256_add_ps(xy,xy), y_0_arr);
            }

            for (size_t j = 0; j < SIZE; j++)
            {
                uint* n_ptr  = (uint*) &n;
                int* cmp_ptr = (int*) &cmp;
                if (!cmp_ptr[j])
                {
                    ((int*)(windowSurface->pixels))[y_i * SCREEN_WIDTH + x_i + j] = *((int*)&colors[n_ptr[j]]);
                }
            }
        }
    }
    return 1;
}
