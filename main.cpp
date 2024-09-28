/**
* Author: Selina Gong
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:58pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 800,
WINDOW_HEIGHT = 550;

constexpr float BG_RED = 0.4f,
BG_GREEN = 0.4f,
BG_BLUE = 0.5f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();


constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

float g_previous_ticks = 0.0f;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL = 0, // mipmap reduction image level
                TEXTURE_BORDER = 0; // this value MUST be zero

// Filepaths for cat pngs
constexpr char CAT1_SPRITE_FILEPATH[] = "crying_cat.png",
               CAT2_SPRITE_FILEPATH[] = "mr_fresh.png";

// Initial scale and position of cats
constexpr glm::vec3 INIT_SCALE_CAT1 = glm::vec3(1.4f, 1.4f, 0.0f),
                    INIT_SCALE_CAT2 = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_CAT1 = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_CAT2 = glm::vec3(-2.0f, 0.0f, 0.0f);

// setting model matrices for cats (and view and proj)
glm::mat4 g_view_matrix,
          g_cat1_model_matrix,
          g_cat2_model_matrix,
          g_projection_matrix;

// const variables for movement
constexpr float ROT_INCREMENT = 1.0f;
    // vars for cat1 move side to side
constexpr float CAT1_MOVE_DIST = 1.0f,
                CAT1_MOVE_SPEED = 1.0f;
    // vars for cat2's circle
constexpr float CAT2_RADIUS_X = 2.2f,
                CAT2_RADIUS_Y = 3.3f;
constexpr float CAT2_ORBIT_SPEED = 1.5f;
    // vars for cat2 scaling
constexpr float CAT2_BASE_AMP = 1.1f,
                CAT2_MAX_AMP = 0.37f,
                CAT2_PULSE_SPEED = 1.46f;


// Accumulators (i think)
    // vectors for rotation
glm::vec3 g_rotation_cat1 = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_cat2 = glm::vec3(0.0f, 0.0f, 0.0f);
    // accumulator angle for moving side-to-side
float g_cat1_mvmt_theta = 0.0f;
    // accumulator offsets for actual x and y of side-to-side
float g_cat1_x_offset = 0.0f,
      g_cat1_y_offset = 0.0f;
    // accumulator angle for cat 2 going circle
float g_cat2_theta = 1.0f;
float g_cat2_x_offset = 0.0f,
      g_cat2_y_offset = 0.0f;
    // accumulator time for cat 2 pulse
float g_cat2_pulse_time = 0.0f;

// texture ids
GLuint g_cat1_texture_id,
       g_cat2_texture_id;


GLuint load_texture(const char* filepath) {
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


static void initialise() {
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("cars go vroom",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    // initializing camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    // loading shaders (need to do this before any other shader stuff)
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    // setting view and projection matrices
    g_view_matrix = glm::mat4(1.0f); // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_cat1_model_matrix = glm::mat4(1.0f); // Defines every translation, rotations, or scaling applied to cat1
    g_cat2_model_matrix = glm::mat4(1.0f); // Defines every translation, rotations, or scaling applied to cat2
    

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    // Each object has its own unique ID
    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // load cat images
    g_cat1_texture_id = load_texture(CAT1_SPRITE_FILEPATH);
    g_cat2_texture_id = load_texture(CAT2_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


static void update() {
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
        // move cat 1 side to side
    g_cat1_mvmt_theta += CAT1_MOVE_SPEED * delta_time;
    g_cat1_x_offset = CAT1_MOVE_DIST * glm::cos(g_cat1_mvmt_theta);
        // rotate cat 1 (flip)
    g_rotation_cat1.y += -1 * ROT_INCREMENT * delta_time;
        // move cat 2 in circle (ellipse)
    g_cat2_theta += CAT2_ORBIT_SPEED * delta_time;
    g_cat2_x_offset = CAT2_RADIUS_X * glm::cos(g_cat2_theta - 10) + g_cat1_x_offset;
    g_cat2_y_offset = CAT2_RADIUS_Y * glm::sin(g_cat2_theta);
    glm::vec3 g_cat2_translation = glm::vec3(g_cat2_x_offset, g_cat2_y_offset, 0.0f);
        // scale cat 2
    g_cat2_pulse_time += CAT2_PULSE_SPEED * delta_time;
    float scale_factor = CAT2_BASE_AMP + CAT2_MAX_AMP * glm::sin(g_cat2_pulse_time);
    glm::vec3 g_cat2_scaling = glm::vec3(scale_factor, scale_factor, 0.0f);


    /* Model matrix reset */
    g_cat1_model_matrix = glm::mat4(1.0f);
    g_cat2_model_matrix = glm::mat4(1.0f);

    /* Transformations */
        // cat 1 (crying cat)
    g_cat1_model_matrix = glm::translate(g_cat1_model_matrix, glm::vec3(g_cat1_x_offset, 0.0f, 0.0f));
    g_cat1_model_matrix = glm::rotate(g_cat1_model_matrix, g_rotation_cat1.y, glm::vec3(0.0f, 1.0f, 0.0f));
    g_cat1_model_matrix = glm::scale(g_cat1_model_matrix, INIT_SCALE_CAT1);

        // cat 2 (mr. fresh)
    g_cat2_model_matrix = glm::translate(g_cat2_model_matrix, g_cat2_translation);
    //g_cat2_model_matrix = glm::rotate(g_cat2_model_matrix,g_rotation_cat2.y,glm::vec3(0.0f, 1.0f, 0.0f));
    g_cat2_model_matrix = glm::scale(g_cat2_model_matrix, g_cat2_scaling);
}


static void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id) {
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


static void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = 
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_cat1_model_matrix, g_cat1_texture_id);
    draw_object(g_cat2_model_matrix, g_cat2_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


static void shutdown() { SDL_Quit(); }



int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }

    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}