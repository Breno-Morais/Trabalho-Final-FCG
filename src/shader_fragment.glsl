#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpola��o da posi��o global e a normal de cada v�rtice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posi��o do v�rtice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no c�digo C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto est� sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define CHEST  3
#define CUBE  4
#define GUN 5
#define STATUEI 6
uniform int object_id;

// Par�metros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Posi��o da Fonte de Luz
uniform vec4 light_pos;
// Dire��o da Fonte de Luz
uniform vec4 light_dir;

// Vari�veis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;

// O valor de sa�da ("out") de um Fragment Shader � a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posi��o da c�mera utilizando a inversa da matriz que define o
    // sistema de coordenadas da c�mera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual � coberto por um ponto que percente � superf�cie de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posi��o no
    // sistema de coordenadas global (World coordinates). Esta posi��o � obtida
    // atrav�s da interpola��o, feita pelo rasterizador, da posi��o de cada
    // v�rtice.
    vec4 p = position_world;
    vec4 pM = position_model;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada v�rtice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em rela��o ao ponto atual.
    vec4 l = normalize(light_pos - p);

    float angle = dot(-l, light_dir);
    float angFech = 30.0f;

    // Vetor que define o sentido da c�mera em rela��o ao ponto atual.
    vec4 v = normalize(camera_position - p);

    vec4 h = normalize(v+l);

    // Vetor que define o sentido da reflex�o especular ideal.
    vec4 r = -l + 2*n*(dot(n,l));

    // Par�metros que definem as propriedades espectrais da superf�cie
    vec3 Kd; // Reflet�ncia difusa
    vec3 Ks; // Reflet�ncia especular
    vec3 Ka; // Reflet�ncia ambiente
    float q; // Expoente especular para o modelo de ilumina��o de Phong

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    if ( object_id == SPHERE )
    {
        // Proje��o Esf�rica de Textura
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
    else if ( object_id == BUNNY )
    {
        // Proje��o de Textura
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (pM.x - minx)/(maxx - minx);
        V = (pM.y - miny)/(maxy - miny);

        // Propriedades espectrais do coelho
        Kd = vec3(0.8,0.8,0.8);
        Ks = vec3(0.8,0.8,0.8);
        Ka = vec3(0.04,0.2,0.4);
        q = 32.0;
    }
    else if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        //U = texcoords.x;
        //V = texcoords.y;

        // Propriedades espectrais do plano
        Kd = vec3(0.05,0.05,0.05);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.2,0.2,0.2);
        q = 10.0;
    }
    else if ( object_id == CHEST)
    {
        Kd = vec3(0.4,0.4,0.4);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.1,0.1,0.1);
        q = 10.0;
    }
    else // Objeto desconhecido
    {
        Kd = vec3(0.4,0.4,0.4);
        Ks = vec3(0.03,0.03,0.03);
        Ka = vec3(0.2,0.2,0.2);
        q = 20.0;
    }

    // Espectro da fonte de ilumina��o
    vec3 I = vec3(1.0,1.0,0.8);

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.01,0.01,0.01);

    // Equa��o de Ilumina��o
    float lambert_diffuse_term = max(0,dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    // Termo especular utilizando o modelo de ilumina��o de Phong
    vec3 blinn_phong_specular_term  = Ks*I*pow(max(0,dot(n,h)), q);

    // Obtemos a reflet�ncia difusa a partir da leitura da imagem TextureImage0
    float ml = pow((1 - lambert_diffuse_term),20);

    // Cor do Pixel da Textura
    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;

    // NOTE: Se voc� quiser fazer o rendering de objetos transparentes, �
    // necess�rio:
    // 1) Habilitar a opera��o de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no c�digo C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *ap�s* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas dist�ncias para a c�mera (desenhando primeiro objetos
    //    transparentes que est�o mais longe da c�mera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    if(acos(angle) < radians(angFech))
        color.rgb = (Kd*I*Kd0*(lambert_diffuse_term + 0.01)) + (texture(TextureImage1, vec2(U,V)).rgb * ml) + ambient_term + blinn_phong_specular_term;
    else color.rgb = ambient_term;
    // 0.01f * (ambient_term + (Kd*Kd0) + texture(TextureImage1, vec2(U,V)).rgb);

    // Cor final com corre��o gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

