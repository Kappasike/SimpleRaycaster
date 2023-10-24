#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

/*** WINDOW WIDTH AND HEIGHT ***/
#define SCREEN_WIDTH 520
#define SCREEN_HEIGHT 320

/*** THE SIMULATED RESOLUTION ***/
#define RES_X 13
#define RES_Y 8

const uint16_t RENDER_RES_X = SCREEN_WIDTH / RES_X;
const uint16_t RENDER_RES_Y = SCREEN_HEIGHT / RES_Y;

#define TILE_PX 64

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
// 13 x 8
#define FOV 75
const float FOV_HALF_RAD = (FOV/2.f)*0.0174533;

const float TWO_PI = M_PI*2;
const float P2 = M_PI/2.f;
const float P3 = 3.f*M_PI/2.f;
const float DEG = ((FOV*0.0174533)/SCREEN_WIDTH)*RENDER_RES_X;

const float yaw_speed = 3.f * .02f;
const float movement_speed = 3.f * .02f;

const uint16_t x_iterations = SCREEN_WIDTH/RENDER_RES_X;

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
} player;

struct {
    SDL_Window* window;
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    uint32_t pixels[SCREEN_WIDTH*SCREEN_HEIGHT]; // array of all pixels
} state;

uint32_t get_color(uint8_t objectType)
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

uint32_t get_tinted_color(uint8_t objectType)
{
    uint32_t ret_color = 0xFF000000;
    switch(objectType)
    {
        case 1:
            ret_color = 0xFF0000BA;
            break;
        case 2:
            ret_color = 0xFF00BA00;
            break;
        case 3:
            ret_color = 0xFFBA0000;
            break;
    }
    return ret_color;
}

/* draw map */
void test_drawmap()
{
    for(int x=0; x<MAP_WIDTH; x++)
    {
        for(int y=0; y<MAP_HEIGHT; y++)
        {
            SDL_Rect dabbington;
            dabbington.w = TILE_PX>>2;
            dabbington.h = TILE_PX>>2;
            dabbington.x = x*(TILE_PX>>2);
            dabbington.y = y*(TILE_PX>>2);
            if(MAP[y*MAP_WIDTH+x]==0)
            {
                SDL_SetRenderDrawColor(state.renderer, 30, 30, 30, 255);
            }
            else if(MAP[y*MAP_WIDTH+x]>=1)
            {
                SDL_SetRenderDrawColor(state.renderer, 0, 255, 0, 255);
            }
            SDL_RenderFillRect(state.renderer, &dabbington);
        }
    }
}

void test_drawplayer()
{
    SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255);
    SDL_Rect cubington_city;
    cubington_city.h = 10;
    cubington_city.w = 10;
    cubington_city.x = ((int)(player.x)-20)>>2;
    cubington_city.y = ((int)(player.y)-20)>>2;
    SDL_RenderFillRect(state.renderer, &cubington_city);
}

/* --- */

float dist_2d(float x1, float x2, float y1, float y2)
{
    return sqrtf((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

/**
* x is column on screen, 0 would be far left column
* h is height of column
*/
void draw_column(int x, int x2, int height, uint32_t color)
{
    int og_height = height;
    height = height / RENDER_RES_Y;
    height = height * RENDER_RES_Y;
    if(abs(og_height-height)>abs(og_height-(height+RENDER_RES_Y)))
    {
        height+=RENDER_RES_Y;
    }
    int offset = (SCREEN_HEIGHT>>1)-((height)>>1);
    for(int i=x; i<x2; i++)
        {
        for(int y=offset; y<offset+height; y++)
        {
            state.pixels[y*SCREEN_WIDTH+i]=color;
        }
    }
}

// possibly the greatest raycasting function of all time
void raycast()
{
    float vx, vy, hx, hy, ray_x, ray_y, x_offset, y_offset, ray_angle, v_angle, h_angle, v_dist, h_dist, lowest_dist;
    int x, map_x, map_y, map, dof;
    uint32_t color = 0xFF000000; // will be the final color
    uint32_t v_color = 0xFF000000; // color if a vertical wall is used
    uint32_t h_color = 0xFF000000; // color if a horizontal wall is used
    ray_angle = player.dir - FOV_HALF_RAD;
    if(ray_angle<0){ray_angle+=TWO_PI;}
    if(ray_angle>TWO_PI){ray_angle-=TWO_PI;}
    for(x=0;x<SCREEN_WIDTH/RENDER_RES_X;x++) // increment through each column of pixels
    {
        dof=0;
        h_dist = 1000000.f;
        hx = player.x;
        hy = player.y;
        v_angle = -1/tan(ray_angle);
        if(ray_angle>M_PI){
            ray_y=(((int)player.y>>6)<<6)-0.0001;
            ray_x=(player.y-ray_y)*v_angle+player.x;
            y_offset=-TILE_PX;
            x_offset=-y_offset*v_angle;
        }
        else if(ray_angle<M_PI){
            ray_y=(((int)player.y>>6)<<6)+TILE_PX;
            ray_x=(player.y-ray_y)*v_angle+player.x;
            y_offset=TILE_PX;
            x_offset=-y_offset*v_angle;
        }
        else
        {
            ray_x=player.x;
            ray_y=player.y;
            dof=8;
        }
        while(dof<8)
        {
            map_x=(int)(ray_x)>>6;
            map_y=(int)(ray_y)>>6;
            map=map_y*MAP_WIDTH+map_x;
            if(map>0 && map<MAP_WIDTH*MAP_HEIGHT && MAP[map]>=1)
            {
                hx = ray_x;
                hy = ray_y;
                h_dist = dist_2d(player.x, hx, player.y, hy);
                h_color = get_tinted_color(MAP[map]);
                dof=8;
            }
            else{
                ray_x+=x_offset;
                ray_y+=y_offset;
                dof++;
            }
        }

        /**** Vertical Part ****/

        dof=0;
        v_dist = 1000000.f;
        vx = player.x;
        vy = player.y;
        h_angle = -tan(ray_angle);
        if(ray_angle>P2 && ray_angle<P3){
            ray_x=(((int)player.x>>6)<<6)-0.0001;
            ray_y=(player.x-ray_x)*h_angle+player.y;
            x_offset=-TILE_PX;
            y_offset=-x_offset*h_angle;
        }
        else if(ray_angle<P2 || ray_angle>P3){
            ray_x=(((int)player.x>>6)<<6)+TILE_PX;
            ray_y=(player.x-ray_x)*h_angle+player.y;
            x_offset=TILE_PX;
            y_offset=-x_offset*h_angle;
        }
        else
        {
            ray_x=player.x;
            ray_y=player.y;
            dof=8;
        }
        while(dof<8)
        {
            map_x=(int)(ray_x)>>6; map_y=(int)(ray_y)>>6; map=map_y*MAP_WIDTH+map_x;
            if(map>0 && map<MAP_WIDTH*MAP_HEIGHT && MAP[map]>=1)
            {
                vx = ray_x;
                vy = ray_y;
                v_dist = dist_2d(player.x, vx, player.y, vy);
                v_color = get_color(MAP[map]);
                dof=8;
            }
            else{
                ray_x+=x_offset; 
                ray_y+=y_offset; 
                dof++;
            }
        }
        if(v_dist<h_dist){
            ray_x=vx;
            ray_y=vy;
            lowest_dist=v_dist;
            color = v_color;
        }
        else{
            ray_x=hx;
            ray_y=hy;
            lowest_dist=h_dist;
            color = h_color;
        }
        float ca = player.dir-ray_angle;
        if(ca<0)
        {
            ca+=TWO_PI;
        }
        if(ca>TWO_PI)
        {
            ca-=TWO_PI;
        } 
        lowest_dist=lowest_dist*cos(ca);
        int height = (TILE_PX*SCREEN_HEIGHT)/lowest_dist;
        if(height>SCREEN_HEIGHT) height = SCREEN_HEIGHT;

        draw_column(x*RENDER_RES_X, ((x+1)*RENDER_RES_X), height, color);

        ray_angle+=DEG;

        if(ray_angle<0){ray_angle+=TWO_PI;}
        if(ray_angle>TWO_PI){ray_angle-=TWO_PI;}
    }
}

int main(int argc, char* args[])
{
    player.dir = M_PI/2;

    //im putting my defenses cause i dont wanna fall in love if i ever did that i think id have a heart attack
    // attttaaaaaaack i think id have a heart attaaaaaacck i think id have a heart attack
    // there's no one else to blame
    // im flying to close to the sun
    // and i burst into flaaaaaaaaaaaaaaAAAAAAAAAAAAAaaaaaaaames
    player.x = 256.f;
    player.y = 256.f;
    
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
                float newX = player.x+cosf(player.dir) * movement_speed*TILE_PX;
                float newY = player.y+sinf(player.dir) * movement_speed*TILE_PX;
                if(MAP[((int)(newY)>>6)*MAP_WIDTH+((int)(newX)>>6)] == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
                
            }

            if(keystate[SDL_SCANCODE_S])
            {
                float newX = player.x-cosf(player.dir) * movement_speed*TILE_PX;
                float newY = player.y-sinf(player.dir) * movement_speed*TILE_PX;
                if(MAP[((int)(newY)>>6)*MAP_WIDTH+((int)(newX)>>6)] == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_D])
            {
                float newX = player.x+cosf(player.dir+M_PI/2) * movement_speed*TILE_PX;
                float newY = player.y+sinf(player.dir+M_PI/2) * movement_speed*TILE_PX;
                if(MAP[((int)(newY)>>6)*MAP_WIDTH+((int)(newX)>>6)] == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_A])
            {
                float newX = player.x-cosf(player.dir+M_PI/2) * movement_speed*TILE_PX;
                float newY = player.y-sinf(player.dir+M_PI/2) * movement_speed*TILE_PX;
                if(MAP[((int)(newY)>>6)*MAP_WIDTH+((int)(newX)>>6)] == 0)
                {
                    player.x = newX;
                    player.y = newY;
                }
            }

            if(keystate[SDL_SCANCODE_Q])
            {
                // i didnt choose this town
                player.dir -= .08f;
                if(player.dir<0)
                {
                    player.dir+=TWO_PI;
                }
            }

            if(keystate[SDL_SCANCODE_E])
            {
                // i dream of getting out
                player.dir += .08f;
                if(player.dir>TWO_PI)
                {
                    player.dir-=TWO_PI;
                }
            }


            memset(state.pixels, 0xFFa8a8a8, sizeof(state.pixels)); // set all the pixels back to black
            raycast();
            SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH*4);
            SDL_RenderCopyEx(state.renderer, state.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
            //test_drawmap();
            //test_drawplayer();
            SDL_RenderPresent(state.renderer);
        }
    }
    
    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    return 0;
}
