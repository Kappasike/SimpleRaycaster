#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

/*** WINDOW WIDTH AND HEIGHT ***/
#define SCREEN_WIDTH 520
#define SCREEN_HEIGHT 320

/*** THE SIMULATED RESOLUTION ***/
#define RES_X 520
#define RES_Y 320

#define ROT_SPEED 0.14
#define MOVEMENT_SPEED 0.06

#define MAP_WIDTH 8
#define MAP_HEIGHT 8

const uint16_t RENDER_RES_X = SCREEN_WIDTH / RES_X;
const uint16_t RENDER_RES_Y = SCREEN_HEIGHT / RES_Y;

const uint16_t RENDER_SCREEN_WIDTH = SCREEN_WIDTH/RENDER_RES_X;

static uint8_t MAP[MAP_WIDTH*MAP_HEIGHT] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 1, 1, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 2, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 1,
    1, 0, 3, 3, 3, 1, 0, 1,
    1, 0, 3, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
};

struct {
    float x;
    float y;
    float dir;
    float plane_x;
    float plane_y; 
} player;

struct {
    SDL_Window* window;
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    uint32_t pixels[SCREEN_WIDTH*SCREEN_HEIGHT]; // array of all pixels
} state;

static inline uint8_t in_block(float x, float y)
{
    return MAP[(int)y*MAP_WIDTH + (int)x];
}

static inline uint32_t get_color(uint8_t objectType)
{
    uint32_t ret_color = 0xFF000000;
    switch(objectType)
    {
        case 1:
            ret_color = 0xFF0000FF;
            break;
        case 2:
            ret_color = 0xFF00FF00;
            break;
        case 3:
            ret_color = 0xFFFF0000;
            break;
    }
    return ret_color;
}

static inline float dist_2d(float x1, float x2, float y1, float y2)
{
    return sqrtf((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

/**
* x is column on screen, 0 would be far left column
* h is height of column
*/
static inline void draw_column(int x1, int x2, int height, uint32_t color)
{
    int original_height = height;
    height = height / RENDER_RES_Y;
    height = height * RENDER_RES_Y;
    if(abs(original_height-height)>abs(original_height-(height+RENDER_RES_Y)))
    {
        height+=RENDER_RES_Y;
    }
    int offset = (SCREEN_HEIGHT>>1)-((height)>>1);
    for(int i=x1; i<x2; i++)
    {
        for(int y=offset; y<offset+height; y++)
        {
            state.pixels[y*SCREEN_WIDTH+i]=color;
        }
    }
}

void render()
{
    for(int x=0; x<RENDER_SCREEN_WIDTH; x++)
    {
        uint8_t map_x = (int)player.x;
        uint8_t map_y = (int)player.y;

        // these are how far we have to initially go to reach a box
        // from the current player position for DDA
        float side_dist_x;
        float side_dist_y;

        // just getting the ray angle and separating it into it's
        // x and y component
        const float camera_pos = 2 * x / (float)(RENDER_SCREEN_WIDTH) - 1;
        const float ray_angle_x = cosf(player.dir) + player.plane_x*camera_pos;
        const float ray_angle_y = sin(player.dir) + player.plane_y*camera_pos;

        // how long of a distance is needed to go from the current tile to the next tile
        // using the ray angle, look up DDA for a concise explanation
        float delta_dist_x = (ray_angle_x==0) ? 1e13 : fabs(1/ray_angle_x);
        float delta_dist_y = (ray_angle_y==0) ? 1e13 : fabs(1/ray_angle_y);

        int8_t step_x;
        int8_t step_y;

        // this represents if we've hit a wall yet
        uint8_t hit = 0;
        bool is_side = false;

        if(ray_angle_x<0) // if facing left then the step direction will be left
        {
            step_x = -1;
            side_dist_x = (player.x - map_x) * delta_dist_x;
        }
        else // if not the step direction will be right
        {
            step_x = 1;
            side_dist_x = (map_x + 1.f - player.x) * delta_dist_x;
        }

        if(ray_angle_y<0) // if facing down then the step direction will be down
        {
            step_y = -1;
            side_dist_y = (player.y - map_y) * delta_dist_y;
        }
        else // if not the step direction will be up
        {
            step_y = 1;
            side_dist_y = (map_y + 1.f - player.y) * delta_dist_y;
        }

        // this is where the DDA algorithm is actually performed
        while(!hit)
        {
            if(side_dist_x<side_dist_y)
            {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                is_side = false;
            }
            else{
                side_dist_y += delta_dist_y;
                map_y += step_y;
                is_side = true;
            }
            hit = in_block(map_x, map_y);
        }
        uint32_t color = get_color(hit);

        // if it's a side then dim the color to create a shading effect
        if(is_side)
        {
            color = color & 0xFFA8A8A8;
        }

        // wall has been hit
        float dist_to_wall = (is_side) ? side_dist_y - delta_dist_y : side_dist_x - delta_dist_x;

        uint16_t height = (int)(SCREEN_HEIGHT / dist_to_wall);
        if(height > SCREEN_HEIGHT) height = SCREEN_HEIGHT;

        draw_column(x*RENDER_RES_X, ((x+1)*RENDER_RES_X), height, color);
    }
}

int main(int argc, char* args[])
{
    // im putting my defenses cause i dont wanna fall in love if i ever did that i think id have a heart attack
    // attttaaaaaaack i think id have a heart attaaaaaacck i think id have a heart attack
    // there's no one else to blame
    // im flying to close to the sun
    // and i burst into flaaaaaaaaaaaaaaAAAAAAAAAAAAAaaaaaaaames
    player.x = 4.f;
    player.y = 4.f;
    player.dir = -M_PI;
    player.plane_x = 0.f;
    player.plane_y = 0.66f;
    
    if(SDL_Init(SDL_INIT_VIDEO)<0)
    {
        printf("Failed to initialize sdl\n");
        return -1;
    }

    state.window = SDL_CreateWindow("KappaTown",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    if(!state.window)
    {
        printf("Failed to make window ):\n");
        return -1;
    }

    state.renderer = SDL_CreateRenderer(state.window, -1, SDL_RENDERER_PRESENTVSYNC);

    if(!state.renderer)
    {
        printf("Failed to make renderer D:\n");
        return -1;
    }

    state.texture = SDL_CreateTexture(state.renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    if(!state.texture)
    {
        printf("Failed to create texture >:\n");
        return -1;
    }

    bool keep_running = true;    
    while(keep_running)
    {
        unsigned char buf[8];
        SDL_Event e;
        while(SDL_PollEvent(&e)>0)
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    keep_running = false;
                    break;
            }
            // updates stuff like player position and rotation
            const uint8_t* keystate = SDL_GetKeyboardState(NULL);
            
            if(keystate[SDL_SCANCODE_W])
            {
                // check if the next input would make us inside a block, if so then don't do anything
                float newX = player.x+cosf(player.dir) * MOVEMENT_SPEED;
                float newY = player.y+sinf(player.dir) * MOVEMENT_SPEED;
                if(in_block(newX, newY) == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
                
            }

            if(keystate[SDL_SCANCODE_S])
            {
                float newX = player.x-cosf(player.dir) * MOVEMENT_SPEED;
                float newY = player.y-sinf(player.dir) * MOVEMENT_SPEED;
                if(in_block(newX, newY) == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_D])
            {
                float newX = player.x-cosf(player.dir+M_PI/2) * MOVEMENT_SPEED;
                float newY = player.y-sinf(player.dir+M_PI/2) * MOVEMENT_SPEED;
                if(in_block(newX, newY) == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_A])
            {
                float newX = player.x+cosf(player.dir+M_PI/2) * MOVEMENT_SPEED;
                float newY = player.y+sinf(player.dir+M_PI/2) * MOVEMENT_SPEED;
                if(in_block(newX, newY) == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_Q])
            {
                // i didnt choose this town
                player.dir += ROT_SPEED;
                float new_plane_x = player.plane_x * cosf(ROT_SPEED) - player.plane_y * sinf(ROT_SPEED);
                float new_plane_y = player.plane_x * sinf(ROT_SPEED) + player.plane_y * cosf(ROT_SPEED);
                player.plane_x = new_plane_x;
                player.plane_y = new_plane_y;
            }

            if(keystate[SDL_SCANCODE_E])
            {
                // i dream of getting out
                player.dir -= ROT_SPEED;
                float new_plane_x = player.plane_x * cosf(-ROT_SPEED) - player.plane_y * sinf(-ROT_SPEED);
                float new_plane_y = player.plane_x * sinf(-ROT_SPEED) + player.plane_y * cosf(-ROT_SPEED);
                player.plane_x = new_plane_x;
                player.plane_y = new_plane_y;
            }

            memset(state.pixels, 0xFFa8a8a8, sizeof(state.pixels));
            render();
            SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH*4);
            SDL_RenderCopyEx(state.renderer, state.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
            SDL_RenderPresent(state.renderer);
        }
    }
    
    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    return 0;
}
