//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   Trabalho FInal
//


#include <cmath>
#include <math.h>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <unordered_map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"

struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

void BuildTrianglesAndAddToVirtualScene(ObjModel* model, bool env = false); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_Parabens(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_ShowCrossHair(GLFWwindow* window);

void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void InputperFrame(GLFWwindow* window);

struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

glm::vec4 camera_position_c = glm::vec4(-1.8f,5.0f,-2.45f,1.0f);
glm::vec4 camera_view_vector = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);

glm::vec4 camera_pos_anim = glm::vec4(0.0f,1.0f,0.0f,1.0f);
glm::vec4 camera_view_anim = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

float g_CameraTheta = -6.25f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.020796f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.5f; // Distância da câmera para a origem
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = false;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint vertex_shader_shadow_id;
GLuint fragment_shader_shadow_id;
GLuint program_id = 0;
GLuint program_shadow_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint lightSpaceMatrix_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;
GLint light_pos_uniform;
GLint light_dir_uniform;
GLint Ka_uniform;
GLint Kd_uniform;
GLint Ks_uniform;
GLint Ke_uniform;

GLuint g_NumLoadedTextures = 0;

glm::vec3 cubic_bezier(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float t);
std::unordered_map<std::string, std::vector<Cubo_Collision>> Cubes_Collisions;
std::unordered_map<std::string, std::vector<Sphere_Collision>> Spheres_Collisions;
std::vector<Plano> Planes_Collisions;
std::vector<std::string> ObjetosCenaNomes;
Cubo Player_AABB {glm::vec4(-0.5f, -1.0f, -0.5f, 1.0f), glm::vec4(0.5f, 10.0f, 0.5f, 1.0f)};
bool Tfinal = false;
Raio ray;
float anim_final = 0.0f;
glm::mat4 anim_model;

static double d2r(double d);
# define M_PI           3.14159265358979323846

int main(int argc, char* argv[])
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;

    // https://gamedev.stackexchange.com/questions/58547/how-to-set-to-fullscreen-in-glfw3
    GLFWmonitor* MyMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    int height = mode->height;
    int width = mode->width;

    window = glfwCreateWindow(width, height, "INF01047 - 335794 - Breno da Silva Morais", MyMonitor, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, width, height); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../data/StoneColor.png"); // TextureImage0
    LoadTextureImage("../data/GoldColor.png"); // TextureImage1
    LoadTextureImage("../data/Grass.jpg"); // TextureImage2

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    glm::vec4 pos, centro, bbox_min, bbox_max;
    std::string Obj_Name;
    float scale = 1.0f;

    Player_AABB.newCentro(camera_position_c);


//-------------------------------------------------------------------

    scale = 0.005f;
    ObjModel spheremodel("../data/snowglobe.obj","../data/");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    pos = glm::vec4(8.44f, 1.8f, 0.61f, 1.0f);
    {
        Sphere_Collision temp =
        {
            {
                pos + glm::vec4(0.0f, 0.2f, 0.01f, 0.0f),
                (norm(g_VirtualScene["sphere"].bbox_min)) * scale
            }, // Esfera
            false,
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale),
            "sphere",
            false,
            0.0f,
            0.0f,
            {
                glm::vec3(pos.x, pos.y, pos.z),
                glm::vec3(pos.x, pos.y + 5.0f, pos.z),
                glm::vec3(-pos.x, pos.y + 5.0f, -pos.z),
                glm::vec3(-pos.x, pos.y, -pos.z)
            },
            glm::vec3(spheremodel.materials[0].ambient[0],spheremodel.materials[0].ambient[1], spheremodel.materials[0].ambient[2]),
            glm::vec3(spheremodel.materials[0].diffuse[0],spheremodel.materials[0].diffuse[1], spheremodel.materials[0].diffuse[2]),
            glm::vec3(spheremodel.materials[0].specular[0],spheremodel.materials[0].specular[1], spheremodel.materials[0].specular[2]),
            glm::vec3(spheremodel.materials[0].transmittance[0],spheremodel.materials[0].transmittance[1], spheremodel.materials[0].transmittance[2])
        };
        Spheres_Collisions["sphere"].push_back(temp);
    }

//-------------------------------------------------------------------

    ObjModel planemodel("../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    {
        Plano temp =
        {
            glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
            25.0f
        };

        Planes_Collisions.push_back(temp);

        temp =
        {
            glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
            20.0f
        };

        Planes_Collisions.push_back(temp);
    }

//-------------------------------------------------------------------
    ObjModel isomodel("../data/iso_flat1piece.obj","../data/");
    ComputeNormals(&isomodel);
    BuildTrianglesAndAddToVirtualScene(&isomodel, true);

    scale = 1.5f;
    Obj_Name = "iso_flat";

    bbox_min = glm::vec4(g_VirtualScene[Obj_Name].bbox_min.x,
                          g_VirtualScene[Obj_Name].bbox_min.y,
                          g_VirtualScene[Obj_Name].bbox_min.z,
                          1.0f);
    bbox_max = glm::vec4(g_VirtualScene[Obj_Name].bbox_max.x,
                          g_VirtualScene[Obj_Name].bbox_max.y,
                          g_VirtualScene[Obj_Name].bbox_max.z,
                          1.0f);

    centro = (bbox_max + bbox_min)/2.0f;
    pos = glm::vec4(5.0f, 1.0f, 1.0f, 1.0f);
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false,
            0.0f,
            0.0f,
            {
                glm::vec3(pos.x, pos.y, pos.z),
                glm::vec3(pos.x, pos.y + 5.0f, pos.z),
                glm::vec3(-pos.x, pos.y + 5.0f, -pos.z),
                glm::vec3(-pos.x, pos.y, -pos.z)
            },
            glm::vec3(isomodel.materials[0].ambient[0],isomodel.materials[0].ambient[1], isomodel.materials[0].ambient[2]),
            glm::vec3(isomodel.materials[0].diffuse[0],isomodel.materials[0].diffuse[1], isomodel.materials[0].diffuse[2]),
            glm::vec3(isomodel.materials[0].specular[0],isomodel.materials[0].specular[1], isomodel.materials[0].specular[2]),
            glm::vec3(isomodel.materials[0].transmittance[0],isomodel.materials[0].transmittance[1], isomodel.materials[0].transmittance[2])
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

//-------------------------------------------------------------------

    ObjModel statuemodel("../data/statue.obj","../data/");
    ComputeNormals(&statuemodel);
    BuildTrianglesAndAddToVirtualScene(&statuemodel);
    Obj_Name = "statue";

    bbox_min = glm::vec4(g_VirtualScene[Obj_Name].bbox_min.x,
                          g_VirtualScene[Obj_Name].bbox_min.y,
                          g_VirtualScene[Obj_Name].bbox_min.z,
                          1.0f);
    bbox_max = glm::vec4(g_VirtualScene[Obj_Name].bbox_max.x,
                          g_VirtualScene[Obj_Name].bbox_max.y,
                          g_VirtualScene[Obj_Name].bbox_max.z,
                          1.0f);

    centro = (bbox_max + bbox_min)/2.0f;

    // GOLDEN
    pos = glm::vec4(19.29f, 0.1f, 9.78f, 1.0f);
    scale = 0.01f;
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false, // Se o objeto está sendo observado
            0.0f, // Tempo sendo visto
            0.0f, // t usado pela curva de bezier
            {
                glm::vec3(pos.x, pos.y + 0.3f, pos.z),
                glm::vec3(18.18f, 7.0f, -5.76f),
                glm::vec3(-18.0f, 5.0f, -4.25f),
                glm::vec3(-14.59f, 0.1f, 13.28f)
            }, // Pontos do caminho de fuga
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

    pos = glm::vec4(-1.68f, 0.2f, -7.15f, 1.0f);
    scale = 0.0045f;
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false, // Se o objeto está sendo observado
            0.0f, // Tempo sendo visto
            0.0f, // t usado pela curva de bezier
            {
                glm::vec3(pos.x, pos.y + 0.57f, pos.z),
                glm::vec3(6.88f, 10.0f, -5.37f),
                glm::vec3(-2.45f, 1.0f, 9.71),
                glm::vec3(7.31f, 0.1f, 1.52)
            } // Pontos do caminho de fuga
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

    pos = glm::vec4(18.74f, 0.2f, -6.72f, 1.0f);
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false, // Se o objeto está sendo observado
            0.0f, // Tempo sendo visto
            0.0f, // t usado pela curva de bezier
            {
                glm::vec3(pos.x, pos.y + 0.57f, pos.z),
                glm::vec3(21.10f, 1.0f, -11.73f),
                glm::vec3(11.73f, 5.0f, -10.43),
                glm::vec3(4.73f, 0.1f, -10.54f)
            } // Pontos do caminho de fuga
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

    pos = glm::vec4(-10.80f, 0.1f, 1.60f, 1.0f);
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false, // Se o objeto está sendo observado
            0.0f, // Tempo sendo visto
            0.0f, // t usado pela curva de bezier
            {
                glm::vec3(pos.x, pos.y + 0.57f, pos.z),
                glm::vec3(4.44f, 5.0f, 27.86f),
                glm::vec3(28.08f, 10.0f, 4.86f),
                glm::vec3(14.32f, 0.1f, 4.33f)
            } // Pontos do caminho de fuga
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

    pos = glm::vec4(-12.71f, 0.1f, 19.09f, 1.0f);
    {
        Cubo_Collision temp = {
            {
                bbox_min,
                bbox_max
            }, // Cube
            false, // bool colide
            Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Scale(scale, scale, scale), // Matrix do Modelo
            Obj_Name, // Nome do objeto na cena virtual
            false, // Se o objeto está sendo observado
            0.0f, // Tempo sendo visto
            0.0f, // t usado pela curva de bezier
            {
                glm::vec3(pos.x, pos.y + 0.57f, pos.z),
                glm::vec3(-0.20f, 5.0f, 12.55f),
                glm::vec3(-14.0f, 2.0f, 6.52f),
                glm::vec3(-3.31f, 0.5f, 4.51f)
            } // Pontos do caminho de fuga
        };
        Cubes_Collisions[Obj_Name].push_back(temp);
    }

//-------------------------------------------------------------------

    ObjModel revolvermodel("../data/revolver.obj");
    ComputeNormals(&revolvermodel);
    BuildTrianglesAndAddToVirtualScene(&revolvermodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Calcula da Iluminação e da posição da câmera
        ray.dir = camera_view_vector;
        ray.origem = camera_position_c;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program_id);

        glm::vec4 direction;
        direction.x = cos(g_CameraPhi)*cos(g_CameraTheta);
        direction.y = sin(g_CameraPhi);
        direction.z = cos(g_CameraPhi)*sin(g_CameraTheta);
        direction.w = 0.0f;

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        InputperFrame(window);

        // Se todas as estátuas foram destruidas
        bool estatua_final = true;
        if(!Spheres_Collisions["sphere"][0].colide) estatua_final = false;
        for(unsigned int i = 1; i < Cubes_Collisions["statue"].size(); i++)
        {
            if(!Cubes_Collisions["statue"][i].colide) estatua_final = false;
        }

        Tfinal = Cubes_Collisions["statue"][0].colide;

        glm::mat4 view;
        glm::vec4 LightPos;
        if((estatua_final && anim_final >= 0) || Tfinal)
        {
            anim_final += deltaTime;
            glm::vec4 newOrigem = glm::vec4(-50.0f, 0.0f, -50.0f, 1.0f);
            anim_model = Matrix_Translate(-50.0f, 0.0f, -50.0f) * Matrix_Rotate_Y(anim_final * 1.22f) * Matrix_Scale(0.002f,0.002f,0.002f);
            camera_pos_anim = newOrigem + glm::vec4(0.0f, 0.5f, 0.35f, 0.0f);
            camera_view_anim = (newOrigem + glm::vec4(0.0f, 0.3f, 0.0f, 0.0f)) - camera_pos_anim;

            view = Matrix_Camera_View(camera_pos_anim, camera_view_anim, camera_up_vector);
            LightPos = newOrigem + glm::vec4(0.0f, 3.0f, 0.0f, 0.0f);
            glm::vec4 LightDir = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
            glUniform4f(light_pos_uniform, LightPos.x, LightPos.y, LightPos.z, 1.0f);
            glUniform4f(light_dir_uniform, LightDir.x, LightDir.y, LightDir.z, 0.0f);

            if((anim_final >= 5) && !Tfinal) anim_final = -1.0f;

        } else
        {
            anim_model = Cubes_Collisions["statue"][0].Matrix_Model;
            camera_view_vector = glm::normalize(direction);
            view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

            // Passa a posição da fonte de luz
            LightPos = camera_position_c + camera_view_vector;
            glUniform4f(light_pos_uniform, LightPos.x, LightPos.y, LightPos.z, 1.0f);

            // Passa a direção da fonte de luz
            glUniform4f(light_dir_uniform, camera_view_vector.x, camera_view_vector.y, camera_view_vector.z, 0.0f);
        }

        glm::mat4 projection;

        float nearplane = -0.001f;  // Posição do "near plane"
        float farplane  = -200.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Desenho dos Objetos
        glm::mat4 model = Matrix_Identity();
        #define SPHERE 0
        #define BUNNY  1
        #define PLANE  2
        #define CHEST  3
        #define CUBE  4
        #define GUN 5
        #define STATUEI 6
        #define STATUEG 7
        #define STATUER 8

        // Desenhamos os modelos das estatuas
        Obj_Name = "statue";
        if(estatua_final)
        {
                Cubo_Collision* modelo = &(Cubes_Collisions[Obj_Name][0]);
                centro = ((modelo->Matrix_Model * modelo->cube.vert_min) + (modelo->Matrix_Model * modelo->cube.vert_max)) * 0.5f;

                glm::vec4 l = (LightPos - centro)/norm(LightPos - centro);
                float angle = dotproduct(-l, camera_view_vector);
                if(modelo->visto)
                {
                    modelo->tempoVisto += deltaTime;

                    glm::vec3 oldPos = glm::vec3(centro.x, centro.y, centro.z);
                    glm::vec3 newPos = cubic_bezier(modelo->Path[0], modelo->Path[1], modelo->Path[2], modelo->Path[3], modelo->t);
                    glm::vec3 deltaPos = newPos - oldPos;
                    modelo->Matrix_Model = Matrix_Translate(deltaPos.x, deltaPos.y, deltaPos.z) * modelo->Matrix_Model;

                    modelo->t = 1 / (1 + exp(-2*(modelo->tempoVisto-5))); // Função Sigmoid

                } else if(acos(angle) < d2r(25))
                {
                    modelo->tempoVisto += deltaTime;
                    if(modelo->tempoVisto >= 5 && !modelo->visto)
                    {
                        modelo->visto = true;
                        modelo->tempoVisto = 0;
                    }
                } else modelo->tempoVisto = 0;

                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(anim_model));
                glUniform1i(object_id_uniform, STATUEG);
                DrawVirtualObject(Obj_Name.c_str());
        }
        else for(unsigned int i = 1; i < Cubes_Collisions[Obj_Name].size(); i++)
        {
            Cubo_Collision* modelo = &(Cubes_Collisions[Obj_Name][i]);
            centro = ((modelo->Matrix_Model * modelo->cube.vert_min) + (modelo->Matrix_Model * modelo->cube.vert_max)) * 0.5f;

            glm::vec4 l = (LightPos - centro)/norm(LightPos - centro);
            float angle = dotproduct(-l, camera_view_vector);
            if(modelo->visto)
            {
                modelo->tempoVisto += deltaTime;

                glm::vec3 oldPos = glm::vec3(centro.x, centro.y, centro.z);
                glm::vec3 newPos = cubic_bezier(modelo->Path[0], modelo->Path[1], modelo->Path[2], modelo->Path[3], modelo->t);
                glm::vec3 deltaPos = newPos - oldPos;
                modelo->Matrix_Model = Matrix_Translate(deltaPos.x, deltaPos.y, deltaPos.z) * modelo->Matrix_Model;

                modelo->t = 1 / (1 + exp(-2*(modelo->tempoVisto-5))); // Função Sigmoid

            } else if(acos(angle) < d2r(25))
            {
                modelo->tempoVisto += deltaTime;
                if(modelo->tempoVisto >= 5 && !modelo->visto)
                {
                    modelo->visto = true;
                    modelo->tempoVisto = 0;
                }
            } else modelo->tempoVisto = 0;


            model = modelo->Matrix_Model;

            if(!(Cubes_Collisions[Obj_Name][i].colide))
            {
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, STATUEI);
                DrawVirtualObject(Obj_Name.c_str());
            }
        }

        // Desenhamos o plano do chão

        model = Matrix_Translate(0.0f,-1.0f,0.0f) * Matrix_Scale(500.0f, 1.0f, 500.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANE);
        DrawVirtualObject("plane");

        // Desenha cenário

        for(std::string Obj_Name: ObjetosCenaNomes)
        {
            for(unsigned int i = 0; i < Cubes_Collisions[Obj_Name].size(); i++)
            {
                Cubo_Collision* modelo = &(Cubes_Collisions[Obj_Name][i]);
                if(!(Cubes_Collisions[Obj_Name][i].colide))
                {
                    glUniform3f(Ka_uniform, modelo->Ka.x, modelo->Ka.y, modelo->Ka.z);
                    glUniform3f(Kd_uniform, modelo->Kd.x, modelo->Kd.y, modelo->Kd.z);
                    glUniform3f(Ks_uniform, modelo->Ks.x, modelo->Ks.y, modelo->Ks.z);
                    glUniform3f(Ke_uniform, modelo->Ke.x, modelo->Ke.y, modelo->Ke.z);
                    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(Cubes_Collisions[Obj_Name][i].Matrix_Model));
                    glUniform1i(object_id_uniform, 99);
                    DrawVirtualObject(Obj_Name.c_str());
                }
            }
        }

        Obj_Name = "sphere";
        Sphere_Collision* modelo = &(Spheres_Collisions[Obj_Name][0]);
        if(!(Spheres_Collisions[Obj_Name][0].colide))
        {
            glUniform3f(Ka_uniform, modelo->Ka.x, modelo->Ka.y, modelo->Ka.z);
            glUniform3f(Kd_uniform, modelo->Kd.x, modelo->Kd.y, modelo->Kd.z);
            glUniform3f(Ks_uniform, modelo->Ks.x, modelo->Ks.y, modelo->Ks.z);
            glUniform3f(Ke_uniform, modelo->Ke.x, modelo->Ke.y, modelo->Ke.z);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(modelo->Matrix_Model));
            glUniform1i(object_id_uniform, SPHERE);
            DrawVirtualObject(Obj_Name.c_str());
        }

        if(!estatua_final || anim_final == -1.0f)
        {
            glDisable(GL_DEPTH_TEST);
            model = Matrix_Translate(0.23f,-1.0f,-2.0f) * Matrix_Scale(0.002f, 0.002f, 0.002f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, GUN);
            DrawVirtualObject("revolver");
            glEnable(GL_DEPTH_TEST);
        }

        TextRendering_ShowFramesPerSecond(window);

        if(Tfinal)
            TextRendering_Parabens(window);
        else
            TextRendering_ShowCrossHair(window);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

glm::vec3 cubic_bezier(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float t)
{
    return (float)(pow((1-t),3)) * p1 + (float)(3*t*pow((1-t),2)) * p2 + (float)(3*t*t*(1-t)) * p3 + (float)(t*t*t)*p4;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../src/shader_fragment.glsl");


    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");
    light_pos_uniform       = glGetUniformLocation(program_id, "light_pos");
    light_dir_uniform       = glGetUniformLocation(program_id, "light_dir");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    Ka_uniform          = glGetUniformLocation(program_id, "MKa");
    Kd_uniform          = glGetUniformLocation(program_id, "MKd");
    Ks_uniform          = glGetUniformLocation(program_id, "MKs");
    Ke_uniform          = glGetUniformLocation(program_id, "MKe");
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model, bool env)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        if(env) ObjetosCenaNomes.push_back(theobject.name);
        printf(theobject.name.c_str());
        std::cout << '\n';

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;
    GLint  number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).

    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
            for(auto& c: Cubes_Collisions)
                for(auto& v: c.second)
                {
                    if(v.objName != "statue") continue;
                    glm::vec4 bbMin = v.Matrix_Model * (glm::vec4(v.cube.vert_min.x, v.cube.vert_min.y, v.cube.vert_min.z, 1.0f));
                    glm::vec4 bbMax = v.Matrix_Model * (glm::vec4(v.cube.vert_max.x, v.cube.vert_max.y, v.cube.vert_max.z, 1.0f));
                    Cubo temp {bbMin, bbMax};
                    v.colide = (collision(ray, temp) || v.colide);

                }

            for(auto& s: Spheres_Collisions)
                for(auto& v: s.second)
                    v.colide = (collision(ray, v.bola) || v.colide);
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void InputperFrame(GLFWwindow* window){
    const float cameraSpeed = 5.2f * deltaTime; // Usa o deltaTime para que a diferença de framerate não afete a velocidade
    const glm::vec4 dir_vec = glm::vec4(camera_view_vector.x, 0.0f, camera_view_vector.z, 0.0f)/norm(glm::vec4(camera_view_vector.x, 0.0f, camera_view_vector.z, 0.0f));

    double mouse_x, mouse_y;
	glfwGetCursorPos(window, &mouse_x, &mouse_y);
	glfwSetCursorPos(window, (double)(800 / 2), (double)(600 / 2));

	float dx = (float)(mouse_x - (double)(800 / 2));

	float dy = (float)(mouse_y - (double)(600 / 2));

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta += 0.01f*dx;
    g_CameraPhi   -= 0.01f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    glm::vec4 nextPos = camera_position_c;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        nextPos = camera_position_c + (cameraSpeed * dir_vec);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        nextPos = camera_position_c - (cameraSpeed * dir_vec);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        nextPos = camera_position_c - (glm::normalize(crossproduct(dir_vec, camera_up_vector)) * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        nextPos = camera_position_c + (glm::normalize(crossproduct(dir_vec, camera_up_vector)) * cameraSpeed);

    Cubo PlayerTemp = Player_AABB;
    PlayerTemp.newCentro(nextPos);

    bool NpodeMover = false;

    if(PlayerTemp.vert_max.x >= 20.0f)
        NpodeMover = true;

    if(PlayerTemp.vert_max.z >= 20.0f)
        NpodeMover = true;

    for(Plano p: Planes_Collisions)
    {
        NpodeMover = collision(PlayerTemp, p) || NpodeMover;
    }

    if(!NpodeMover)
    {
        camera_position_c = nextPos;
        Player_AABB.newCentro(camera_position_c);
    }
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

void TextRendering_ShowCrossHair(GLFWwindow* window)
{
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    TextRendering_PrintString(window, "+", 0.0f-(charwidth/2.0f), 0.0f-(lineheight/2), 1.0f);
}

void TextRendering_Parabens(GLFWwindow* window)
{
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    TextRendering_PrintString(window, "PARABENS", 0.0f-((8*charwidth)/2.0f), 0.0f-(lineheight/2), 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

static double d2r(double d) {
  return (d / 180.0) * ((double) M_PI);
}
