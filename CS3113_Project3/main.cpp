/**
* Author: Garvin Huang
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1


#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <ctime>
#include "cmath"




// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH = 900,
WINDOW_HEIGHT = 700;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

constexpr int FONTBANK_SIZE = 16;

constexpr char LANDER_FILEPATH[] = "assets/kirby.png",
               FIRE_FILEPATH[] = "assets/fire.png",
               BRICK_FILEPATH[] = "assets/brick.png",
               FINISH_FILEPATH[] = "assets/Finish_Line.png",
               FONT_FILEPATH[] = "assets/font1.png";


constexpr float ACC_OF_GRAVITY = -0.5f,
                THRUSTER_POWER = 1.25f,
                DRAG_COEFF = -0.5f,
                FUEL_USAGE = 0.01f;

                
constexpr glm::vec3 LANDER_SCALE = glm::vec3(0.4f, 0.4f, 1.0f);

constexpr glm::vec3 START_POS = glm::vec3(3.75f, 0.0f, 0.0f),
                    FINISH_POS = glm::vec3(-3.0f, 0.0f, 0.0f);

constexpr GLint NUMBER_OF_TEXTURES = 1,
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;


constexpr int   FINISH_NUM = 1,
                FIRE_NUM = 23,
                BRICK_NUM = 1,
                PLATFORM_NUM = FINISH_NUM + FIRE_NUM + BRICK_NUM;

constexpr char WIN_MESSAGE[] = "MISSION ACCOMPLISHED!!!",
               LOSE_MESSAGE[] = "MISSION FAILED!!!";


// ————— STRUCTS AND ENUMS —————//
enum AppStatus { RUNNING, TERMINATED };

struct GameState
{
    Entity* player;
    Entity* collidables;
};

// ————— VARIABLES ————— //
GameState g_game_state;


SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

bool g_game_over = false;

GLuint g_fire_texture_id,
       g_finish_texture_id,
       g_font_texture_id;

const char* g_game_over_message;

float g_fuel = 100.0f; 


// ---- PROTOTYPES ---- //
void initialise();
void process_input();
void update();
void render();
void shutdown();
GLuint load_texture(const char* filepath);
void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position);



// ———— GENERAL FUNCIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}


void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Kirby Lander",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

   

;   // ---- COLLIDABLES ---- //
    g_fire_texture_id = load_texture(FIRE_FILEPATH);
    g_finish_texture_id = load_texture(FINISH_FILEPATH);
    GLuint brick_texture_id = load_texture(BRICK_FILEPATH);
    g_game_state.collidables = new Entity[PLATFORM_NUM];
    
    g_game_state.collidables[0] = Entity(g_finish_texture_id, 0.0f, 1.0f, 0.35f);
    g_game_state.collidables[0].set_position(FINISH_POS);

    g_game_state.collidables[1] = Entity(brick_texture_id, 0.0f, 1.0f, 1.0f);
    g_game_state.collidables[1].set_position(START_POS);

    int curr = 2, counter = 0;
    for (int x = -3; x <= 3; ++x  ) {
        for (int y = -3; y <= 3; ++y) {
            if (curr >= PLATFORM_NUM) { break; }
            if (counter % 2 == 1 && (x != FINISH_POS.x || y != FINISH_POS.y)) {
                g_game_state.collidables[curr] = Entity(g_fire_texture_id, 0.0f, 0.7f, 0.8f);
                g_game_state.collidables[curr].set_position(glm::vec3(x, y, 0.0f));
                ++curr;
            }
            ++counter;
        }
    }

    for (int i = 0; i < PLATFORM_NUM; ++i) {
        g_game_state.collidables[i].update(0, nullptr, 0);
    }
    // ————— PLAYER ————— //
    GLuint player_texture_id = load_texture(LANDER_FILEPATH);
    g_game_state.player = new Entity();
    g_game_state.player->set_texture_id(player_texture_id);
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
    g_game_state.player->set_position(glm::vec3(3.5f, 1.0f, 0.0f));
    g_game_state.player->set_scale(LANDER_SCALE);
    // ---- TEXT ---- //
    g_font_texture_id = load_texture(FONT_FILEPATH);
    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q: g_app_status = TERMINATED;
            default:     break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (g_fuel <= 0) { return; }
    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
        g_fuel -= FUEL_USAGE;
        
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {   
        g_game_state.player->move_right();
        g_fuel -= FUEL_USAGE;
        
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_game_state.player->move_up();
        g_fuel -= FUEL_USAGE;
   
    }


    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        g_game_state.player->normalise_movement();
}


void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP)
    {
       
        glm::vec3 player_accel = g_game_state.player->get_acceleration(),
                    thruster_accel = g_game_state.player->get_movement()
                                    * THRUSTER_POWER;
        float player_xvel = g_game_state.player->get_velocity().x;
        float drag = player_xvel * DRAG_COEFF; //pseudo-drag
                    
        
        player_accel = thruster_accel + glm::vec3( drag, ACC_OF_GRAVITY , 0.0f);
          
        g_game_state.player->set_acceleration(player_accel);
        g_game_state.player->update(delta_time,g_game_state.collidables, PLATFORM_NUM);
        Entity* collided_with = g_game_state.player->get_collided_with();
        if (collided_with != nullptr) 
        {
            GLuint collided_texture_id = collided_with->get_texture_id();
           
            if (collided_texture_id == g_fire_texture_id || 
                (g_fuel <= 0 && collided_texture_id != g_finish_texture_id))
            {
                g_game_over = true;
                g_game_over_message = LOSE_MESSAGE;
            }
            else if (collided_texture_id == g_finish_texture_id
                     && g_game_state.player->get_collided_bottom())
            { 
                g_game_over = true;
                g_game_over_message = WIN_MESSAGE; 
            }
            
            
        }
        if (g_fuel <= 0 && g_game_state.player->get_position().y <= -3.75)
        {
            g_game_over = true;
            g_game_over_message = LOSE_MESSAGE;
        }
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;
}

   


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    
    for (int i = 0; i < PLATFORM_NUM; ++i) {
        g_game_state.collidables[i].render(&g_shader_program);
    }
    g_game_state.player->render(&g_shader_program);
    if (g_fuel <= 0) {
        draw_text(&g_shader_program, g_font_texture_id, "Out of Fuel", 0.25f, 0.025f,
            glm::vec3(2.0f, -3.0f, 0.0f));
    }
    if (g_game_over){
        draw_text(&g_shader_program, g_font_texture_id, g_game_over_message, 0.25f, 0.025f,
            glm::vec3(-2.0f, 2.0f, 0.0f));
    }
   
    glFlush();
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();
    delete   g_game_state.player;
    delete[] g_game_state.collidables;
}


int main(int argc, char* argv[])
{
    initialise();
   
    while (g_app_status == RUNNING)
    {
        process_input();
        if (!g_game_over) {
            update(); 
            render();
        }
        
    }

    shutdown();
    return 0;
}