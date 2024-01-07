#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_btn_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void changeImguiMode(GLFWwindow* window);
AABB calculateAABB(std::vector<Mesh>& meshes);
void changeCurrentModel(const std::string& direction);
// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 7.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
Camera backupCamera;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//imgui mode -> if true we are not moving the camera
bool shiftKeyPressed = false; //this + mouserightclick activates/deactivates imgui mode
bool imguiMode = false;
int newModels = 0;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//models
std::vector<ModelData> models;
//current model -> used to move the current model using wasd when not in camera mode
ModelData currentModel = {};
int currentModelIndex = -1;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Room Planner", /*glfwGetPrimaryMonitor()*/NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_btn_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader wallShader("wall_vertex.vert", "wall_fragment.frag");
    Shader modelShader("model_vertex.vert", "model_fragment.frag");

    float length = 0.0f, width = 0.0f;
    bool walls_created = false;
    float* wall_vertices;
    GLuint VAO_walls, VBO_walls, EBO_walls;
    glGenVertexArrays(1, &VAO_walls);
    glGenBuffers(1, &VBO_walls);
    glGenBuffers(1, &EBO_walls);



    std::vector<GLuint> wall_indices{
        //face wall
        2,3,7,
        2,6,7,
        //rear wall
        0,1,5,
        0,4,5,
        //left wall
        0,2,6,
        0,4,6,
        //right wall
        1,3,7,
        1,5,7,
        //floor
        0,1,2,
        1,2,3
    };



    glm::vec3 translate(glm::vec3(0.0f, 0.3f * 2.0f, 0.0f));
    // load models
    // -----------
    ModelData ourModel = {
        Model((FileSystem::getPath("resources/objects/coffeetable/coffeetable.obj"))),
        translate,                  // translation vector
        0.0f,                       // rotation angle
        glm::vec3(0.003f * 1.0f),     // scaling vector
        AABB(),
        true,
    };

    ourModel.boundingBox = calculateAABB(ourModel.model.meshes);

    // render loop
    // -----------

    const float minLength = 5.0f; // Minimum length value
    const float minWidth = 5.0f;  // Minimum width value
    const float minHeight = 3.0f; // Minimum height value

    bool printed = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        //initialize imgui window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //add components to imgui window
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Controls:");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Shift+LeftClick to enable/disable cursor.");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "\nWith cursor disabled:\n\tWASD and mouse to move camera.");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "\n\nWith cursor enabled:\n\tWASD to move current model.");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "\n\tLeft bracket \"[\" to select previous model.");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "\n\tRight bracket \"]\" to select next model.");

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "\n\nApplication avg %.3f ms/frame (%.1f FPS)\n\n", 1000.0f / io.Framerate, io.Framerate);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Current model index: %d", currentModelIndex);
            ImGui::SliderFloat3("Translation", &translate.x, -100.0f, 100.0f);

            
            // ImGui input fields


            // Clamp values to minimum values
            length = (length < minLength) ? minLength : length;
            width = (width < minWidth) ? minWidth : width;
            if (!walls_created) {
                ImGui::Text("Input wall dimensions:\n");
                ImGui::InputFloat("Length", &length, 0.1f, 1.0f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
                ImGui::InputFloat("Width", &width, 0.1f, 1.0f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
                if (ImGui::Button("Create walls")) {
                    walls_created = true;
                    wall_vertices = new float[24] {
                        //floor points
                        -length / 2, 0.0f, width / 2, //top left
                            length / 2, 0.0f, width / 2,  //top right
                            -length / 2, 0.0f, -width / 2, //bottom left
                            length / 2, 0.0f, -width / 2, //bottom right

                            //roof points
                            -length / 2, 3.0f, width / 2, //top left
                            length / 2, 3.0f, width / 2,  //top right
                            -length / 2, 3.0f, -width / 2, //bottom left
                            length / 2, 3.0f, -width / 2, //bottom right
                    };

                    glBindVertexArray(VAO_walls);

                    glBindBuffer(GL_ARRAY_BUFFER, VBO_walls);
                    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), wall_vertices, GL_STATIC_DRAW);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_walls);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wall_indices) * sizeof(GLuint), wall_indices.data(), GL_STATIC_DRAW);

                    wallShader.use();
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);

                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    glBindVertexArray(0);
                }
            }
            if (walls_created) {
                if (ImGui::Button("Add test model")) { //generate model add button only if walls are created 
                    ModelData newModel = ourModel;
                    newModel.translate = glm::vec3(models.size() * 1.0f, 0.0f, 0.0f);
                    models.push_back(newModel);
                    newModels++;

                    if (!currentModel.valid) {
                        currentModelIndex = 0;
                        currentModel = models[currentModelIndex];
                        models.erase(models.begin());
                    }
                    else if (currentModel.valid) {
                        models.insert(models.begin() + currentModelIndex, currentModel);
                        currentModel = models[currentModelIndex = models.size() - 1];
                        models.erase(models.begin() + currentModelIndex);
                    }
                }
                if (currentModel.valid && ImGui::Button("Remove current model")) {
                    if (currentModelIndex > 0) {
                        currentModelIndex--;
                        currentModel = models[currentModelIndex];
                        models.erase(models.begin() + currentModelIndex);
                    }
                    else if (currentModelIndex == 0) {
                        if (models.size() > 0) {
                            currentModelIndex = 0;
                            currentModel = models[currentModelIndex];
                            models.erase(models.begin());
                        }
                        else {
                            currentModelIndex = -1;
                            currentModel = {};
                        }
                    }
                }
            }
            ImGui::Text("Currently loaded %d test models", currentModel.valid ? models.size() + 1 : models.size());



            if (imguiMode && currentModel.valid) {
                if (ImGui::Button("Rotate Left")) {
                    currentModel.angle -= 5.0f; 
                }
                ImGui::SameLine();
                if (ImGui::Button("Rotate Right")) {
                    currentModel.angle += 5.0f; 
                }
            }
        }
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        if (walls_created) {
            wallShader.use();
            wallShader.setMat4("projection", projection);
            wallShader.setMat4("view", view);
            wallShader.setMat4("model", glm::mat4(1.0f));
            glBindVertexArray(VAO_walls);
            glDrawElements(GL_TRIANGLES, wall_indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);


        // render the loaded models
        glm::mat4 modelMatrix;
        for (int i = 0; i < models.size(); i++) {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, translate);
            modelMatrix = glm::translate(modelMatrix, models[i].translate);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].angle), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMatrix = glm::scale(modelMatrix, models[i].scale);	// it's a bit too big for our scene, so scale it down
            modelShader.setMat4("model", modelMatrix);
            models[i].model.Draw(modelShader);
        }
        // render the current model
        if (currentModel.valid) {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, translate);
            modelMatrix = glm::translate(modelMatrix, currentModel.translate);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(currentModel.angle), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMatrix = glm::scale(modelMatrix, currentModel.scale);	// it's a bit too big for our scene, so scale it down
            modelShader.setMat4("model", modelMatrix);
            currentModel.model.Draw(modelShader);
            if (!printed)
                std::printf("min %f %f %f max %f %f %f", currentModel.boundingBox.minCorner.x, currentModel.boundingBox.minCorner.y, currentModel.boundingBox.minCorner.z,
                    currentModel.boundingBox.maxCorner.x, currentModel.boundingBox.maxCorner.y, currentModel.boundingBox.maxCorner.z);
            printed = true;
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------

static bool leftBracketPressed = false;
static bool rightBracketPressed = false;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!imguiMode) {  //camera mode
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    else if (imguiMode) {
        if (currentModel.valid) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                currentModel.translate.z -= 1.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                currentModel.translate.z += 1.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                currentModel.translate.x -= 1.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                currentModel.translate.x += 1.0f * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS && !leftBracketPressed) {
            leftBracketPressed = true;

            if (currentModelIndex == -1) {
                currentModelIndex = 0;
                currentModel = models[currentModelIndex];
                models.erase(models.begin());
            }
            changeCurrentModel("left");
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_RELEASE) {
            leftBracketPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS && !rightBracketPressed) {
            rightBracketPressed = true;

            if (currentModelIndex == -1) {
                currentModelIndex = 0;
                currentModel = models[currentModelIndex];
                models.erase(models.begin());
            }
            changeCurrentModel("right");
        }
        else if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_RELEASE) {
            rightBracketPressed = false;
        }
        
    }
}

void changeCurrentModel(const std::string& direction) {
    if (models.size() > 0) {
        if (direction == "left") {
            if (currentModelIndex > 0) {
                models.insert(models.begin() + currentModelIndex, currentModel);
                currentModel = models[--currentModelIndex];
                models.erase(models.begin() + currentModelIndex);
            }
            else if(currentModelIndex==-1){
                currentModelIndex = 0;
                currentModel = models[currentModelIndex];
                models.erase(models.begin());
            }
        }
        else {
            if (currentModelIndex > -1 && currentModelIndex < models.size()) {
                models.insert(models.begin() + currentModelIndex, currentModel);
                currentModel = models[++currentModelIndex];
                models.erase(models.begin() + currentModelIndex);
            }
            else if(currentModelIndex==-1){
                currentModelIndex = models.size() - 1;
                currentModel = models[currentModelIndex];
                models.erase(models.end()-1);
            }
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    if (!imguiMode) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void mouse_btn_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT)) {
        changeImguiMode(window);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void changeImguiMode(GLFWwindow* window)
{
    imguiMode = !imguiMode;

    if (imguiMode) {
        backupCamera = camera;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        camera = backupCamera;
    }
}

AABB calculateAABB(std::vector<Mesh>& meshes) {
    glm::vec3 minCorner(std::numeric_limits<float>::max());
    glm::vec3 maxCorner(std::numeric_limits<float>::lowest());

    for each (Mesh m in meshes)
    {
        std::vector<Vertex> vertices = m.vertices;

        for each (Vertex vertex in vertices) {
            // Update the minimum corner
            minCorner.x = std::min(minCorner.x, vertex.Position.x);
            minCorner.y = std::min(minCorner.y, vertex.Position.y);
            minCorner.z = std::min(minCorner.z, vertex.Position.z);

            // Update the maximum corner
            maxCorner.x = std::max(maxCorner.x, vertex.Position.x);
            maxCorner.y = std::max(maxCorner.y, vertex.Position.y);
            maxCorner.z = std::max(maxCorner.z, vertex.Position.z);
        }
    }
    return AABB(minCorner, maxCorner);
}