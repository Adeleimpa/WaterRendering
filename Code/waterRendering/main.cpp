// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include "common/controls.hpp"

// in order to use stb_image.h and import a png texture image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Plane.h"
#include "GLTexture.h"
#include "Texture.h"
#include "Camera.h"
#include "SceneObject.h"
#include "MeshObject.h"
#include "WaterCube.h"
#include "Aquarium.h"
#include "Light.h"
#include "Skybox.h"
#include "WaterFrameBuffers.h"

GLFWwindow* window;

void key (GLFWwindow *window, int key, int scancode, int action, int mods );

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera *camera = new Camera();
glm::vec3 camera_position = glm::vec3(0.0f, 0.0f,  3.0f);

float angle_in_degrees = 1.;
bool cameraRotates = false;
float cameraSpeed;
bool speedUp = false;
bool slowDown = false;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// scene objects
std::vector<SceneObject*> scene_objects;

//rotation
float angle = 0.;
float zoom = 1.;

// light
glm::vec3 light_I_a =glm::vec3(0.8f, 0.8f, 0.8f);
glm::vec3 light_I_d = glm::vec3(0.8f, 0.8f, 0.8f);
glm::vec3 light_I_s = glm::vec3(0.9f, 0.9f, 0.9f);
glm::vec3 light_pos = glm::vec3(0.0f, 7.0f, 1.5f);
glm::vec3 light_color = glm::vec3(1.0f, 1.0f,  1.0f);
Light *light = new Light(light_I_a, light_I_d, light_I_s, light_pos, light_color);

// plane to put the aquarium on top of it
Plane *plane = new Plane(3.0, 3.0, 10, 10, glm::vec3(0.0,-1.01,0.0), 1); // plane in y=0

// skybox
Skybox *skybox = new Skybox(glm::vec3(0.0,0.0,0.0), 100.0);

// water
WaterCube *water = new WaterCube(glm::vec3(0.0,0.0,0.0), 2.0);

// aquarium
Aquarium *aquarium = new Aquarium(water->side_len, 2,  glm::vec3(0.0,0.0,0.0));

// textures
GLTexture *texture = new GLTexture();

// draw wired mesh or not
bool wired = false;

// reflection camera
Camera *reflection_camera = new Camera();
glm::vec3 refl_cam_position = glm::vec3(camera_position[0], camera_position[1], camera_position[2]);
float refl_cam_vertical_angle = 3.14f/4.0f;


GLuint programID;
/*******************************************************************************/


void enableBlending() {
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
}


int main( void )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    enableBlending(); // for transparency


    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);

    // key
    glfwSetKeyCallback(window, key);
    int screen_width, screen_height;
    glfwGetFramebufferSize(window, &screen_width, &screen_height);

    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );


    // default to orbital camera
    setCamPosition(glm::vec3( 0, 5, 5));
    setVerticalAngle(-3.14f/4.0f);


    // ------------------------------------------------------------------------------------
    // PLANE
    // ------------------------------------------------------------------------------------
    // generate plane -> fill arrays of indices, triangles and indexed_vertices
    plane->generatePlane(1.0);
    plane->generateBuffers();

    plane->setColor(glm::vec3(0.5, 0.27, 0.11));
    plane->setMaterial(glm::vec3(0.2f, 0.1f, 0.0f), glm::vec3(0.6f, 0.3f, 0.0f), glm::vec3(0.1f, 0.1f, 0.1f), 0.);
    plane->isSkybox = true; // to test texture
    // ------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------
    // SKYBOX
    // -----------------------------------------------------------------------------------_
    skybox->generatePlanes();
    skybox->setCubeColor(glm::vec3(0.0f, 0.8f, 0.9f));
    skybox->setCubeMaterial(glm::vec3(0.2f, 0.1f, 0.0f), glm::vec3(0.6f, 0.3f, 0.0f), glm::vec3(0.1f, 0.1f, 0.1f), 0.);
    skybox->setIsSkybox(1);
    // ------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------
    // WATER
    // ------------------------------------------------------------------------------------
    water->generateBuffers();
    water->setColor(glm::vec3(0.67, 0.84, 0.9));
    water->setMaterial(glm::vec3(0.0f, 0.5f, 0.7f), glm::vec3(0.0f, 0.5f, 0.7f), glm::vec3(0.5f, 0.5f, 0.5f), 0.5);
    // ------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------
    // AQUARIUM
    // ------------------------------------------------------------------------------------
    aquarium->setAquariumColor(glm::vec3(0.94, 0.94, 0.94));
    aquarium->generatePlanes();
    aquarium->setAquariumMaterial(glm::vec3(0.9f, 0.9f, 0.9f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.9f, 0.9f, 0.9f), 0.2);
    // ------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------
    // Add objects to scene_objects
    // Attention importance de l'ordre pour la transparence
    //scene_objects.push_back(sky);
    scene_objects.push_back(skybox->top);
    scene_objects.push_back(skybox->floor);
    scene_objects.push_back(skybox->left);
    scene_objects.push_back(skybox->right);
    scene_objects.push_back(skybox->back);
    scene_objects.push_back(skybox->front);

    scene_objects.push_back(plane);

    scene_objects.push_back(aquarium->floor);
    scene_objects.push_back(aquarium->left);
    scene_objects.push_back(aquarium->right);
    scene_objects.push_back(aquarium->back);

    scene_objects.push_back(water);
    // ------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------
    // TEXTURES
    // ------------------------------------------------------------------------------------
    texture->generateTexture();
    texture->loadTexture((char*)"textures/texture.png");
    texture->defineParameters();
    // ------------------------------------------------------------------------------------


    // reflection refraction
    refl_cam_position[1] += 2 * (camera_position[1] - water->getWaterHeight()); // Mirror the y-coordinate
    //setCamPosition(refl_cam_position);
    //setVerticalAngle(refl_cam_vertical_angle);
    //WaterFrameBuffers *water_fbs = new WaterFrameBuffers();

    do{

        enableBlending(); // for transparency

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // send light data to shaders
        light->sendDataToShaders(programID);

        // Use our shader
        glUseProgram(programID);

        // ---------------
        // RENDER REFLECTION OFF SCREEN TO MAKE TEXTURE
        /*water_fbs->bindReflectionFrameBuffer();

        // CAMERA
        reflection_camera->MVP(cameraRotates, speedUp, slowDown);
        reflection_camera->sendMVPtoShader(programID);
        glUniform3f(glGetUniformLocation(programID, "viewPos"), refl_cam_position[0], refl_cam_position[1], refl_cam_position[2]); // todo pas sure

        // Draw the triangles !
        for(int i = 0; i < scene_objects.size(); i++){

            if(scene_objects[i]->isSkybox==1){ // skybox
                // send textures to shader
                texture->sendTextureToShader(programID, "skybox_txt", 0);
            }

            // todo ne pas render water for relfexion

            scene_objects[i]->loadBuffers();
            scene_objects[i]->draw(programID, wired);
        }

        water_fbs->unbindCurrentFrameBuffer();*/
        // ---------------

        // ---------------
        // RENDER SCENE AS NORMAL

        // CAMERA
        camera->MVP(cameraRotates, speedUp, slowDown);
        speedUp = false;
        slowDown = false;
        camera->sendMVPtoShader(programID);
        glUniform3f(glGetUniformLocation(programID, "viewPos"), camera_position[0], camera_position[1], camera_position[2]);

        // Restore default framebuffer and viewport
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        glUseProgram(programID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, water_fbs->getReflectionTexture()); 
        glUniform1i(glGetUniformLocation(programID, "reflectionTexture"), 0); 

        // animate water
        float amplitude = 0.12f * sin(glfwGetTime());  // Example: amplitude changes over time
        float frequency = 1.8f;  // Example: constant frequency
        water->animateWater(amplitude, frequency, currentFrame);

        // Draw the triangles !
        for(int i = 0; i < scene_objects.size(); i++){

            if(scene_objects[i]->isSkybox==1){ // skybox
                // send textures to shader
                texture->sendTextureToShader(programID, "skybox_txt", 0);
            }

            scene_objects[i]->loadBuffers();
            scene_objects[i]->draw(programID, wired);
        }
        // ---------------


        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    for(SceneObject *obj: scene_objects){
        obj->deleteBuffers();
    }
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


void key (GLFWwindow *window, int key, int scancode, int action, int mods ) {

    /*if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        std::cout << "Key pressed: " << key << std::endl;
    }*/


    if( key == GLFW_KEY_C and action == GLFW_PRESS ){
        /// turn around axis

        if(cameraRotates){
            cameraRotates = false;
            setCamPosition(glm::vec3( 0, 0.55, 5));
            setVerticalAngle(0.0f);
            std::cout << "You have pressed the key C : switch to normal camera" << std::endl;
        }else{
            cameraRotates = true;
            setCamPosition(glm::vec3( 0, 5, 5));
            setVerticalAngle(-3.14f/4.0f);
            std::cout << "You have pressed the key C : switch to orbital camera" << std::endl;
        }

    }else if( key == GLFW_KEY_SLASH and action == GLFW_PRESS ){ // + on macbook keyboard
        std::cout << "You have pressed the key + : rotation speeds up" << std::endl;
        /// accelerates camera
        speedUp = true;
    }else if ( key == GLFW_KEY_EQUAL and action == GLFW_PRESS ) { // - on macbook keyboard
        std::cout << "You have pressed the key - : rotation slows down" << std::endl;
        /// slows down camera
        slowDown = true;
    }


    else if( key == GLFW_KEY_Z and action == GLFW_PRESS ){ // W on macbook keyboard

        if(wired){
            wired = false;
            std::cout << "You have pressed the key W : wire mode deactivated" << std::endl;
        }else{
            wired = true;
            std::cout << "You have pressed the key W : wire mode activated" << std::endl;
        }
    }


}
