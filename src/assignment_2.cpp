#include <cstdlib>
#include <iostream>

#include "mygl/shader.h"
#include "mygl/model.h"
#include "mygl/camera.h"

#include "boat.h"
#include "water.h"



namespace BoatSpotLights{

    enum eSpotlight { FRONT_LEFT = 0, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT, SPOTLIGHT_COUNT };
    struct Spotlight {
        Vector3D position;
        Vector3D direction;
        float cutOffAngle= 30 * M_PI / 180.0;
        Vector3D color;
    } ;

    struct Spotlight spotlights[SPOTLIGHT_COUNT];

};

struct
{
    Camera camera;
    bool cameraFollowBoat;
    float zoomSpeedMultiplier;

    Boat boat;
    Model water;
    ShaderProgram shaderBoat;
    ShaderProgram shaderWater;

    WaterSim waterSim;
} sScene;

struct
{
    bool mouseButtonPressed = false;
    Vector2D mousePressStart;
    bool keyPressed[Boat::eControl::CONTROL_COUNT] = {false, false, false, false, false, false};
} sInput;


namespace LightTime {

    const  Vector3D dirLightSourceDirection = {1, 1, 0};
    const  Vector3D dayLightColor = {1, 1, 1};
    const Vector3D nightLightColor = {0.1, 0.1, 0.3};
    Vector3D currentLightColor=dayLightColor;

};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /* input for camera control */
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = false;
        sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
        cameraUpdateOrbit(sScene.camera, {0.0f, 0.0f}, 0.0f);
    }
    if(key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = false;
    }
    if(key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = true;
    }
    //input for day and night control.
    if(key == GLFW_KEY_3) {
        sInput.keyPressed[Boat::DAY] = (action == GLFW_PRESS);
    }
    if(key == GLFW_KEY_4)
    {
        sInput.keyPressed[Boat::NIGHT] = (action == GLFW_PRESS);
    }

    /* input for boat control */
    if(key == GLFW_KEY_W)
    {
        sInput.keyPressed[Boat::eControl::THROTTLE_UP] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_S)
    {
        sInput.keyPressed[Boat::eControl::THROTTLE_DOWN] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if(key == GLFW_KEY_A)
    {
        sInput.keyPressed[Boat::eControl::RUDDER_LEFT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_D)
    {
        sInput.keyPressed[Boat::eControl::RUDDER_RIGHT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    /* close window on escape */
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        screenshotToPNG("screenshot.png");
    }
}

void mousePosCallback(GLFWwindow* window, double x, double y)
{
    if(sInput.mouseButtonPressed)
    {
        Vector2D diff = sInput.mousePressStart - Vector2D(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        sInput.mouseButtonPressed = (action == GLFW_PRESS);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraUpdateOrbit(sScene.camera, {0, 0}, sScene.zoomSpeedMultiplier * yoffset);
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
}

Vector3D multiplyMat4dAndVec3d(const Matrix4D& matrix, const Vector3D& vector) {
    Vector4D vector4D = Vector4D{vector, 0.0}; // Convert to 4D vector
    Vector4D result4D = matrix * vector4D; // Multiply with the matrix
    return Vector3D{result4D.x, result4D.y, result4D.z}; // Convert back to 3D vector
}

void sceneInit(float width, float height)
{
    sScene.camera = cameraCreate(width, height, to_radians(45.0), 0.01, 500.0, {10.0, 10.0, 10.0}, {0.0, 0.0, 0.0});
    sScene.cameraFollowBoat = true;
    sScene.zoomSpeedMultiplier = 0.05f;

    sScene.boat = boatLoad("assets/boat/boat.obj");
    sScene.water = modelLoad("assets/water/water.obj").front();

    sScene.shaderBoat = shaderLoad("src/shader/default.vert", "src/shader/color.frag");
    sScene.shaderWater = shaderLoad("src/shader/default.vert", "src/shader/color.frag");

    Vector3D direction = Vector3D{0.0, 0.0, 1.0}; // Pointing forward
    Vector3D color = {1.0,1.0,1.0};
    Matrix4D boatTransformation = sScene.boat.transformation;
    Vector3D TransformedDirection = multiplyMat4dAndVec3d(boatTransformation, direction); //adjust direction with movement of boat.
    float angle = 45 * M_PI / 180.0;

    // Set properties of spotlight:

    for(int i = 0; i < BoatSpotLights::SPOTLIGHT_COUNT; ++i) {
        BoatSpotLights::Spotlight& spotlight = BoatSpotLights::spotlights[i];
        if(i == BoatSpotLights::FRONT_LEFT) {
            Vector3D position = sScene.boat.position + Vector3D{-1.0, 2.0, -1.2}; // Adjust the y value based on the height of the cabin
            spotlight.color=color;
            spotlight.direction=TransformedDirection;
            spotlight.position=position;
            spotlight.cutOffAngle=angle;
        }
        else if(i == BoatSpotLights::FRONT_RIGHT) {
            Vector3D position = sScene.boat.position + Vector3D{1.0, 2.0, -1.2}; // Adjust the y value based on the height of the cabin
            spotlight.color=color;
            spotlight.direction=TransformedDirection;
            spotlight.position=position;
            spotlight.cutOffAngle=angle;
        }
    }


}

void sceneUpdate(float dt)
{
    sScene.waterSim.accumTime += dt;

    boatMove(sScene.boat, sScene.waterSim, sInput.keyPressed, dt);

    if (!sScene.cameraFollowBoat)
        cameraFollow(sScene.camera, sScene.boat.position);

    if(sInput.keyPressed[Boat::DAY])
    {
        LightTime::currentLightColor = LightTime::dayLightColor;
    }
    else if(sInput.keyPressed[Boat::NIGHT])
    {
        LightTime::currentLightColor = LightTime::nightLightColor;
    }
}


void render()
{
    /* setup camera and model matrices */
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);
    Vector3D dirLightSourceDirection = LightTime::dirLightSourceDirection;
    Vector3D lightColor;
    lightColor = LightTime::currentLightColor;


    //render Boat
    glUseProgram(sScene.shaderBoat.id);
    shaderUniform(sScene.shaderBoat, "uProj",  proj);
    shaderUniform(sScene.shaderBoat, "uView",  view);
    shaderUniform(sScene.shaderBoat, "uModel",  sScene.boat.transformation);
    shaderUniform(sScene.shaderBoat, "uCameraPos", sScene.camera.position);


    //Directional light
    shaderUniform(sScene.shaderBoat, "uLightDirectionalDir", dirLightSourceDirection);
    shaderUniform(sScene.shaderBoat, "uLightColor", lightColor);

   //spotlights values
    Vector3D position = sScene.boat.position + Vector3D{0.0, 2.0, -1.2}; // Adjust the y value based on the height of the cabin
    Vector3D direction = Vector3D{0.0, 0.0, 1.0}; // Pointing forward
    Vector3D color = {1.0,1.0,1.0};
    Matrix4D boatTransformation = sScene.boat.transformation;
    Vector3D rotatedDirection = multiplyMat4dAndVec3d(boatTransformation, direction); //adjust direction with movement of boat.
    float angle = 45 * M_PI / 180.0;

        //boat spotlights
    for (int i = 0; i < 4; ++i) {
        shaderUniform(sScene.shaderBoat, "uSpotlights[" + std::to_string(i) + "].position", position);
        shaderUniform(sScene.shaderBoat, "uSpotlights[" + std::to_string(i) + "].direction", rotatedDirection);
        shaderUniform(sScene.shaderBoat, "uSpotlights[" + std::to_string(i) + "].color", color);
        shaderUniform(sScene.shaderBoat, "uSpotlights[" + std::to_string(i) + "].angle",angle);
    }



    for(unsigned int i = 0; i < sScene.boat.partModel.size(); i++)
    {
        auto& model = sScene.boat.partModel[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(sScene.shaderBoat, "uModel", sScene.boat.transformation);

        for(auto& material : model.material)
        {
            /* set material properties */
            shaderUniform(sScene.shaderBoat, "uMaterial.ambient", material.ambient);
            shaderUniform(sScene.shaderBoat, "uMaterial.diffuse", material.diffuse);
            shaderUniform(sScene.shaderBoat, "uMaterial.specular", material.specular);
            shaderUniform(sScene.shaderBoat, "uMaterial.shininess", material.shininess);
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
        }
    }

    /* render water */

        glUseProgram(sScene.shaderWater.id);

        /* setup camera and model matrices */
        shaderUniform(sScene.shaderWater, "uProj",  proj);
        shaderUniform(sScene.shaderWater, "uView",  view);
        shaderUniform(sScene.shaderWater, "uModel", Matrix4D::identity());
    shaderUniform(sScene.shaderWater, "uTime", sScene.waterSim.accumTime);
        shaderUniform(sScene.shaderBoat, "uLightDirectionalDir", dirLightSourceDirection);
        shaderUniform(sScene.shaderBoat, "uLightColor", lightColor);

        /* set material properties */
        shaderUniform(sScene.shaderWater, "uMaterial.diffuse", sScene.water.material.front().diffuse);
        shaderUniform(sScene.shaderWater, "uMaterial.ambient", sScene.water.material.front().ambient);
        shaderUniform(sScene.shaderBoat, "uMaterial.specular", sScene.water.material.front().specular);
        shaderUniform(sScene.shaderBoat, "uMaterial.shininess", sScene.water.material.front().shininess);

    for (int i = 0; i < 4; ++i) {
        shaderUniform(sScene.shaderWater, "uSpotlights[" + std::to_string(i) + "].position", position);
        shaderUniform(sScene.shaderWater, "uSpotlights[" + std::to_string(i) + "].direction", rotatedDirection);
        shaderUniform(sScene.shaderWater, "uSpotlights[" + std::to_string(i) + "].color", color);
        shaderUniform(sScene.shaderWater, "uSpotlights[" + std::to_string(i) + "].angle",angle);
    }

        glBindVertexArray(sScene.water.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.water.material.front().indexCount, GL_UNSIGNED_INT, (const void*) (sScene.water.material.front().indexOffset*sizeof(unsigned int)) );

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}


void sceneDraw()
{
    glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /*------------ render scene -------------*/
    render();

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

int main(int argc, char** argv)
{
    /*---------- init window ------------*/
    int width = 1280;
    int height = 720;
    GLFWwindow* window = windowCreate("Assignment 2 - Shader Programming", width, height);
    if(!window) { return EXIT_FAILURE; }

    /* set window callbacks */
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* setup scene */
    sceneInit(width, height);

    /*-------------- main loop ----------------*/
    double timeStamp = glfwGetTime();
    double timeStampNew = 0.0;
    while(!glfwWindowShouldClose(window))
    {
        /* poll and process input and window events */
        glfwPollEvents();

        /* update scene */
        timeStampNew = glfwGetTime();
        sceneUpdate(timeStampNew - timeStamp);
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }


    /*-------- cleanup --------*/
    boatDelete(sScene.boat);
    modelDelete(sScene.water);
    shaderDelete(sScene.shaderBoat);
    shaderDelete(sScene.shaderWater);
    windowDelete(window);

    return EXIT_SUCCESS;
}
