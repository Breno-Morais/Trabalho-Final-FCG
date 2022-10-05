#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

in vec3 cor;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define CHEST  3
#define CUBE  4
#define GUN 5
#define STATUEI 6
#define STATUEG 7
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Posição da Fonte de Luz
uniform vec4 light_pos;
// Direção da Fonte de Luz
uniform vec4 light_dir;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform vec3 MKa;
uniform vec3 MKd;
uniform vec3 MKs;
uniform vec3 MKe;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;
    vec4 pM = position_model;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(light_pos - p);

    float angle = dot(-l, light_dir);
    float angFech = 30.0f;

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    vec4 h = normalize(v+l);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*(dot(n,l));

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    if ( object_id == SPHERE )
    {
        // Projeção Esférica de Textura
        float theta = 0.0;
        float phi = 0.0;

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        theta = atan(pM.x, pM.z);
        phi = asin(pM.y/length(pM - bbox_center));

        U = (theta + M_PI) / (2*M_PI);
        V = (phi + (M_PI / 2)) / M_PI;

        // Propriedades espectrais da esfera
        Kd = vec3(0.8,0.4,0.08);
        Ks = vec3(0.2,0.2,0.2);
        Ka = vec3(0.4,0.2,0.04);
        q = 20.0;
    }
    else if ( object_id == PLANE )
    {
        // Propriedades espectrais do plano
        Kd = vec3(0.015,0.02,0.00);
        Ks = vec3(0.2,0.2,0.0);
        Ka = vec3(0.2,0.2,0.2);
        q = 80.0;
    }
    else if ( object_id == CHEST)
    {
        Kd = vec3(0.4,0.4,0.4);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.1,0.1,0.1);
        q = 10.0;
    }
    else if( object_id == STATUEI || object_id == STATUEG )
    {
        U = texcoords.x;
        V = texcoords.y;

        // Propriedades espectrais do coelho
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.03,0.03,0.03);
        Ka = vec3(0.1,0.1,0.1);
        q = 20.0;
    }
    else if(object_id == 99)
    {
        Ka = (MKa * 0.1f) + vec3(0.2f, 0.2f, 0.0f);
        Kd = (MKd * 0.1f) + vec3(0.01f, 0.01f, 0.0f);
        Ks = (MKs * 0.1f) + vec3(0.01f, 0.01f, 0.0f);
    }
    else // Objeto desconhecido
    {
        U = texcoords.x;
        V = texcoords.y;

        Kd = vec3(0.3,0.3,0.3);
        Ks = vec3(0.03,0.03,0.03);
        Ka = vec3(0.2,0.2,0.2);
        q = 20.0;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,0.8);

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.01,0.01,0.01);

    // Equação de Iluminação
    float lambert_diffuse_term = max(0,dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 blinn_phong_specular_term  = Ks*I*pow(max(0,dot(n,h)), q);

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    float ml = pow((1 - lambert_diffuse_term),20);

    // Cor do Pixel da Textura
    color.a = 1;

    if(object_id == GUN)
        color.rgb = ambient_term + (Kd*I*lambert_diffuse_term*0.01f) + blinn_phong_specular_term*0.01f;
    else if(object_id == SPHERE)
    {
        color.rgb = cor;
    }
    else if(acos(angle) < radians(angFech))
    {
        if(object_id == STATUEI)
            color.rgb = texture(TextureImage0, vec2(U,V)).rgb*lambert_diffuse_term + ambient_term + blinn_phong_specular_term;
        else if(object_id == STATUEG)
            color.rgb = texture(TextureImage1, vec2(U,V)).rgb*lambert_diffuse_term + ambient_term + blinn_phong_specular_term;
        else color.rgb = Kd*I*(lambert_diffuse_term + 0.01) + ambient_term + blinn_phong_specular_term;
    }
    else
    {
        if(object_id == STATUEI)
            color.rgb = (texture(TextureImage0, vec2(U,V)).rgb*lambert_diffuse_term + ambient_term + blinn_phong_specular_term)*0.01f;
        else if(object_id == STATUEG)
            color.rgb = (texture(TextureImage1, vec2(U,V)).rgb*lambert_diffuse_term + ambient_term + blinn_phong_specular_term)*0.01f;
        else color.rgb = ambient_term + (Kd*I*lambert_diffuse_term*0.01f);
    }

    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

