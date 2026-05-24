#include "Render.h"
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "ObjLoader.h"
#include "Texture.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <iostream>
#include <sstream>


#include "debout.h"

// Внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

bool texturing = true;
bool lightning = true;
bool alpha = false;

// Переключение режимов освещения, текстурирования, альфа-наложения
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    // Конвертируем код клавиши в букву
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

    switch (key)
    {
    case 'L':
        lightning = !lightning;
        break;
    case 'T':
        texturing = !texturing;
        break;
    case 'A':
        alpha = !alpha;
        break;
    }
}

// Умножение матриц c[M1][N1] = a[M1][N1] * b[M2][N2]
template <typename T, int M1, int N1, int M2, int N2> void MatrixMultiply(const T* a, const T* b, T* c)
{
    for (int i = 0; i < M1; ++i)
    {
        for (int j = 0; j < N2; ++j)
        {
            c[i * N2 + j] = T(0);
            for (int k = 0; k < N1; ++k)
            {
                c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
            }
        }
    }
}

// Текстовый прямоугольник в верхнем правом углу.
// OGL не предоставляет возможности для хранения текста;
// внутри этого класса создается картинка с текстом (через GDI),
// в виде текстуры накладывается на прямоугольник и рисуется на экране.
// Это самый простой, но очень неэффективный способ написать что-либо на экране.
GuiTextRectangle text;

// ID для текстуры
GLuint texId;

ObjModel lamp_model, fan_model, bed_model, desk_model, pcblock_model, mouse_model, keyboard_model, chair_model, plant_model, carpet_model;

Shader cassini_sh;
Shader phong_sh;
Shader vb_sh;
Shader simple_texture_sh;

Texture plintus, wood_floor, stena, potolok, lamp, door, bed, desk, desktop, pcblock, kovrik, mouse, keyboard, chair, plant, carpet;
// Выполняется один раз перед первым рендером
void initRender()
{
    // Настройка шейдеров
    cassini_sh.VshaderFileName = "shaders/v.vert";
    cassini_sh.FshaderFileName = "shaders/cassini.frag";
    cassini_sh.LoadShaderFromFile();
    cassini_sh.Compile();

    phong_sh.VshaderFileName = "shaders/v.vert";
    phong_sh.FshaderFileName = "shaders/light.frag";
    phong_sh.LoadShaderFromFile();
    phong_sh.Compile();

    vb_sh.VshaderFileName = "shaders/v.vert";
    vb_sh.FshaderFileName = "shaders/vb.frag";
    vb_sh.LoadShaderFromFile();
    vb_sh.Compile();

    simple_texture_sh.VshaderFileName = "shaders/v.vert";
    simple_texture_sh.FshaderFileName = "shaders/textureShader.frag";
    simple_texture_sh.LoadShaderFromFile();
    simple_texture_sh.Compile();

    plintus.LoadTexture("textures/plintus.jpg");
    potolok.LoadTexture("textures/potolok.jpg");
    lamp.LoadTexture("textures/lamp.png");
    stena.LoadTexture("textures/stena.png");
    wood_floor.LoadTexture("textures/wood_floor.png");
    door.LoadTexture("textures/door.png");
    bed.LoadTexture("textures/bed.jpg");
    desk.LoadTexture("textures/desk.jpg");
    desktop.LoadTexture("textures/desktop.png");
    pcblock.LoadTexture("textures/pcblock.jpg");
    kovrik.LoadTexture("textures/kovrik.jpg");
    mouse.LoadTexture("textures/mouse.jpg");
    keyboard.LoadTexture("textures/keyboard.jpg");
    chair.LoadTexture("textures/chair.jpg");
    plant.LoadTexture("textures/plant.jpg");
    carpet.LoadTexture("textures/carpet.jpg");

    lamp_model.LoadModel("models//lamp.obj");
    fan_model.LoadModel("models/fan.obj");
    bed_model.LoadModel("models/bed.obj");
    desk_model.LoadModel("models/desk.obj");
    pcblock_model.LoadModel("models/pcblock.obj");
    mouse_model.LoadModel("models/mouse.obj");
    keyboard_model.LoadModel("models/keyboard.obj");
    chair_model.LoadModel("models/chair.obj");
    plant_model.LoadModel("models/plant.obj");
    carpet_model.LoadModel("models/carpet.obj");
    //==============НАСТРОЙКА ТЕКСТУР================
    // 4 байта на хранение пикселя
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    //================НАСТРОЙКА КАМЕРЫ======================
    camera.caclulateCameraPos();

    // привязываем камеру к событиям "движка"
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
    //==============НАСТРОЙКА СВЕТА===========================
    // Привязываем свет к событиям "движка"
    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
    //========================================================
    //====================Прочее==============================
    gl.KeyDownEvent.reaction(switchModes);
    text.setSize(512, 180);
    //========================================================

    camera.setPosition(2, 1.5, 1.5);
}

float view_matrix[16];
double full_time = 0;
int location = 0;
float fanAngle = 0.0f;
bool computerOn = false;
bool pcKeyPressed = false;

void Render(double delta_time)
{
    full_time += delta_time;

    fanAngle += delta_time * 250.0f;
    if (fanAngle > 360.0f) fanAngle -= 360.0f;

    if (GetAsyncKeyState(0x31))
    {
        if (!pcKeyPressed)
        {
            computerOn = !computerOn;
            pcKeyPressed = true;
        }
    }
    else
    {
        pcKeyPressed = false;
    }

    // Настройка камеры и света
    camera.SetUpCamera();
    // Забираем матрицу MODELVIEW сразу после установки камеры,
    // так как в ней отсутствуют трансформации glRotate
    glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

    light.SetUpLight();

    // Рисуем оси
    gl.DrawAxes();

    glBindTexture(GL_TEXTURE_2D, 0);

    // Включаем нормализацию нормалей
    // чтобы glScaled не влияли на них.

    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Переключаем режимы (см void switchModes(OpenGL *sender, KeyEventArg arg))
    if (lightning)
        glEnable(GL_LIGHTING);
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0); // Сбрасываем текущую текстуру
    }

    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    //=============НАСТРОЙКА МАТЕРИАЛА==============

    float roomAmb[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float roomDif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float roomSpec[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, roomAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, roomDif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, roomSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 8.0f);

    glColor4f(1, 1, 1, 1);

    // размеры комнаты
    double left = -5.0;
    double right = 5.0;
    double bottom = -5.0;
    double top = 5.0;
    double back = -8.0;
    double front = 8.0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);

    // пол
    wood_floor.Bind();

    glColor4d(1, 1, 1, 1);

    glBegin(GL_QUADS);
    glNormal3d(0, 0, 1);

    glTexCoord2d(0, 0);
    glVertex3d(left, back, bottom);

    glTexCoord2d(4, 0);
    glVertex3d(right, back, bottom);

    glTexCoord2d(4, 4);
    glVertex3d(right, front, bottom);

    glTexCoord2d(0, 4);
    glVertex3d(left, front, bottom);

    glEnd();

    // потолок
    potolok.Bind();

    glBegin(GL_QUADS);
    glNormal3d(0, 0, -1);

    glTexCoord2d(0, 0);
    glVertex3d(left, front, top);

    glTexCoord2d(4, 0);
    glVertex3d(right, front, top);

    glTexCoord2d(4, 4);
    glVertex3d(right, back, top);

    glTexCoord2d(0, 4);
    glVertex3d(left, back, top);

    glEnd();

    // левая стена
    stena.Bind();

    glBegin(GL_QUADS);
    glNormal3d(1, 0, 0);

    glTexCoord2d(0, 0);
    glVertex3d(left, back, bottom);

    glTexCoord2d(4, 0);
    glVertex3d(left, front, bottom);

    glTexCoord2d(4, 2);
    glVertex3d(left, front, top);

    glTexCoord2d(0, 2);
    glVertex3d(left, back, top);

    glEnd();

    // правая стена
    glBegin(GL_QUADS);
    glNormal3d(-1, 0, 0);

    glTexCoord2d(0, 0);
    glVertex3d(right, front, bottom);

    glTexCoord2d(4, 0);
    glVertex3d(right, back, bottom);

    glTexCoord2d(4, 2);
    glVertex3d(right, back, top);

    glTexCoord2d(0, 2);
    glVertex3d(right, front, top);

    glEnd();

    // задняя стена
    glBegin(GL_QUADS);
    glNormal3d(0, 1, 0);

    glTexCoord2d(0, 0);
    glVertex3d(right, back, bottom);

    glTexCoord2d(4, 0);
    glVertex3d(left, back, bottom);

    glTexCoord2d(4, 2);
    glVertex3d(left, back, top);

    glTexCoord2d(0, 2);
    glVertex3d(right, back, top);

    glEnd();

    // передняя стена
    stena.Bind();

    glColor4d(1, 1, 1, 1);

    glBegin(GL_QUADS);
    glNormal3d(0, -1, 0);

    glTexCoord2d(0, 0);
    glVertex3d(left, front, bottom);

    glTexCoord2d(4, 0);
    glVertex3d(right, front, bottom);

    glTexCoord2d(4, 2);
    glVertex3d(right, front, top);

    glTexCoord2d(0, 2);
    glVertex3d(left, front, top);
    glEnd();

    
    // мебель
    
    // стол
    glEnable(GL_TEXTURE_2D);
    desk.Bind();

    glColor3f(1, 1, 1);

    glPushMatrix();

    glTranslated(-3.7, -5.2, -4.95);

    glRotated(90, 0, 0, 1);

    glScaled(2.3, 2.3, 2.3);

    glBegin(GL_QUADS);

    glNormal3d(0, 0, 1);
    glTexCoord2d(0, 0);
    glVertex3d(-1.2, -0.5, 1.0);
    glTexCoord2d(2, 0);
    glVertex3d(1.2, -0.5, 1.0);
    glTexCoord2d(2, 1);
    glVertex3d(1.2, 0.5, 1.0);
    glTexCoord2d(0, 1);
    glVertex3d(-1.2, 0.5, 1.0);

    glNormal3d(0, 0, -1);
    glTexCoord2d(0, 0);
    glVertex3d(-1.2, -0.5, 0.9);
    glTexCoord2d(0, 1);
    glVertex3d(-1.2, 0.5, 0.9);
    glTexCoord2d(2, 1);
    glVertex3d(1.2, 0.5, 0.9);
    glTexCoord2d(2, 0);
    glVertex3d(1.2, -0.5, 0.9);

    glNormal3d(0, -1, 0);
    glTexCoord2d(0, 0);
    glVertex3d(-1.2, -0.5, 0.9);
    glTexCoord2d(2, 0);
    glVertex3d(1.2, -0.5, 0.9);
    glTexCoord2d(2, 1);
    glVertex3d(1.2, -0.5, 1.0);
    glTexCoord2d(0, 1);
    glVertex3d(-1.2, -0.5, 1.0);

    glNormal3d(0, 1, 0);
    glTexCoord2d(0, 0);
    glVertex3d(-1.2, 0.5, 0.9);
    glTexCoord2d(0, 1);
    glVertex3d(-1.2, 0.5, 1.0);
    glTexCoord2d(2, 1);
    glVertex3d(1.2, 0.5, 1.0);
    glTexCoord2d(2, 0);
    glVertex3d(1.2, 0.5, 0.9);

    glNormal3d(-1, 0, 0);
    glTexCoord2d(0, 0);
    glVertex3d(-1.2, -0.5, 0.9);
    glTexCoord2d(1, 0);
    glVertex3d(-1.2, -0.5, 1.0);
    glTexCoord2d(1, 1);
    glVertex3d(-1.2, 0.5, 1.0);
    glTexCoord2d(0, 1);
    glVertex3d(-1.2, 0.5, 0.9);

    glNormal3d(1, 0, 0);
    glTexCoord2d(0, 0);
    glVertex3d(1.2, -0.5, 0.9);
    glTexCoord2d(1, 0);
    glVertex3d(1.2, 0.5, 0.9);
    glTexCoord2d(1, 1);
    glVertex3d(1.2, 0.5, 1.0);
    glTexCoord2d(0, 1);
    glVertex3d(1.2, -0.5, 1.0);
    glEnd();

    auto drawTableLeg = [](double x, double y)
        {
            double s = 0.07;
            double h = 0.9;

            glBegin(GL_QUADS);

            glNormal3d(0, -1, 0);
            glTexCoord2d(0, 0);
            glVertex3d(x - s, y - s, 0);
            glTexCoord2d(1, 0);
            glVertex3d(x + s, y - s, 0);
            glTexCoord2d(1, 1);
            glVertex3d(x + s, y - s, h);
            glTexCoord2d(0, 1);
            glVertex3d(x - s, y - s, h);

            glNormal3d(0, 1, 0);
            glTexCoord2d(0, 0);
            glVertex3d(x + s, y + s, 0);
            glTexCoord2d(1, 0);
            glVertex3d(x - s, y + s, 0);
            glTexCoord2d(1, 1);
            glVertex3d(x - s, y + s, h);
            glTexCoord2d(0, 1);
            glVertex3d(x + s, y + s, h);

            glNormal3d(-1, 0, 0);
            glTexCoord2d(0, 0);
            glVertex3d(x - s, y - s, 0);
            glTexCoord2d(1, 0);
            glVertex3d(x - s, y + s, 0);
            glTexCoord2d(1, 1);
            glVertex3d(x - s, y + s, h);
            glTexCoord2d(0, 1);
            glVertex3d(x - s, y - s, h);

            glNormal3d(1, 0, 0);
            glTexCoord2d(0, 0);
            glVertex3d(x + s, y + s, 0);
            glTexCoord2d(1, 0);
            glVertex3d(x + s, y - s, 0);
            glTexCoord2d(1, 1);
            glVertex3d(x + s, y - s, h);
            glTexCoord2d(0, 1);
            glVertex3d(x + s, y + s, h);

            glNormal3d(0, 0, -1);
            glTexCoord2d(0, 0);
            glVertex3d(x - s, y - s, 0);
            glTexCoord2d(1, 0);
            glVertex3d(x - s, y + s, 0);
            glTexCoord2d(1, 1);
            glVertex3d(x + s, y + s, 0);
            glTexCoord2d(0, 1);
            glVertex3d(x + s, y - s, 0);

            glNormal3d(0, 0, 1);
            glTexCoord2d(0, 0);
            glVertex3d(x - s, y - s, h);
            glTexCoord2d(1, 0);
            glVertex3d(x + s, y - s, h);
            glTexCoord2d(1, 1);
            glVertex3d(x + s, y + s, h);
            glTexCoord2d(0, 1);
            glVertex3d(x - s, y + s, h);

            glEnd();
        };

    glColor3f(1, 1, 1);

    drawTableLeg(-1.05, -0.35);
    drawTableLeg(1.05, -0.35);
    drawTableLeg(-1.05, 0.35);
    drawTableLeg(1.05, 0.35);

    glPopMatrix();

    // ковер
    glPushMatrix();

    glTranslated(1.0, 2.6, -4.95);

    glScaled(0.037, 0.037, 0.037);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    carpet.Bind();
    carpet_model.Draw();

    glPopMatrix();

    // стул
    glPushMatrix();

    glTranslated(-2.4, -4.6, -4.95);
    glRotated(-90, 0, 0, 1);
    glScaled(0.03, 0.03, 0.03);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    chair.Bind();
    chair_model.Draw();

    glPopMatrix();
    
    // цветок
    glPushMatrix();

    glTranslated(-4.4, -2.9, -2.65); 
    glRotated(90, 0, 0, 1);
    glScaled(0.01, 0.01, 0.01);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    plant.Bind();
    plant_model.Draw();

    glPopMatrix();
    
    // монитор
    glPushMatrix();

    glDisable(GL_LIGHTING);

    glBindTexture(GL_TEXTURE_2D, 0);

    glTranslated(-4.2, -5.2, -5.7);
    glRotated(90, 0, 0, 1);
    glScaled(3.0, 3.0, 3.0);

    glTranslated(0.0, 0.0, 1.02);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.12f, 0.12f, 0.12f);

    glBegin(GL_QUADS);

    glVertex3d(-0.14, -0.09, 0.0);
    glVertex3d(0.14, -0.09, 0.0);
    glVertex3d(0.14, 0.09, 0.0);
    glVertex3d(-0.14, 0.09, 0.0);

    glEnd();

    glBegin(GL_QUADS);

    glVertex3d(-0.02, -0.02, 0.0);
    glVertex3d(0.02, -0.02, 0.0);
    glVertex3d(0.02, 0.02, 0.25);
    glVertex3d(-0.02, 0.02, 0.25);

    glEnd();

    glTranslated(0, 0, 0.38);

    double w = 0.35;
    double h = 0.22;
    double t = 0.015;

    glColor3f(0.08f, 0.08f, 0.09f);

    glBegin(GL_QUADS);

    glVertex3d(-w, 0, -h);
    glVertex3d(w, 0, -h);
    glVertex3d(w, 0, h);
    glVertex3d(-w, 0, h);

    glVertex3d(-w, t, -h);
    glVertex3d(w, t, -h);
    glVertex3d(w, t, h);
    glVertex3d(-w, t, h);

    glVertex3d(-w, 0, h);
    glVertex3d(w, 0, h);
    glVertex3d(w, t, h);
    glVertex3d(-w, t, h);

    glVertex3d(-w, 0, -h);
    glVertex3d(w, 0, -h);
    glVertex3d(w, t, -h);
    glVertex3d(-w, t, -h);

    glVertex3d(-w, 0, -h);
    glVertex3d(-w, t, -h);
    glVertex3d(-w, t, h);
    glVertex3d(-w, 0, h);

    glVertex3d(w, 0, -h);
    glVertex3d(w, t, -h);
    glVertex3d(w, t, h);
    glVertex3d(w, 0, h);

    glEnd();

    if (computerOn)
    {
        glEnable(GL_TEXTURE_2D);
        desktop.Bind();
        glColor3f(1, 1, 1);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.02f, 0.02f, 0.02f);
    }

    glBegin(GL_QUADS);

    if (computerOn) glTexCoord2d(0, 0);
    glVertex3d(-0.30, -0.001, -0.17);

    if (computerOn) glTexCoord2d(1, 0);
    glVertex3d(0.30, -0.001, -0.17);

    if (computerOn) glTexCoord2d(1, 1);
    glVertex3d(0.30, -0.001, 0.17);

    if (computerOn) glTexCoord2d(0, 1);
    glVertex3d(-0.30, -0.001, 0.17);

    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();

    // системник
    
    glPushMatrix();

    glTranslated(-3.7, -7.1, -2.7); 
    glRotated(90, 0, 0, 1);  
    glScaled(0.035, 0.035, 0.035);

    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);

    pcblock.Bind();
    pcblock_model.Draw();

    glPopMatrix();

   
    // коврик
    
    glPushMatrix();

    glTranslated(-3.35, -4.8, -2.648);
    glRotated(90, 0, 0, 1);

    glScaled(3.4, 1.1, 1.5);

    double z = 0.001;

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    kovrik.Bind();

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);

    glTexCoord2d(0, 0);
    glVertex3d(-0.5, -0.5, z);

    glTexCoord2d(1, 0);
    glVertex3d(0.5, -0.5, z);

    glTexCoord2d(1, 1);
    glVertex3d(0.5, 0.5, z);

    glTexCoord2d(0, 1);
    glVertex3d(-0.5, 0.5, z);

    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float rgbTime = full_time * 1.5f;

    float r = 0.5f + 0.5f * sin(rgbTime);
    float g = 0.5f + 0.5f * sin(rgbTime + 2.094f);
    float b = 0.5f + 0.5f * sin(rgbTime + 4.188f);

    float border = 0.03f;

    glBegin(GL_QUADS);

    glColor4f(r, g, b, 1.0f);
    glVertex3d(-0.5, -0.5, z);

    glVertex3d(0.5, -0.5, z);

    glColor4f(r, g, b, 0.0f);
    glVertex3d(0.5, -0.5 - border, z);

    glVertex3d(-0.5, -0.5 - border, z);

    glEnd();

    glBegin(GL_QUADS);

    glColor4f(r, g, b, 1.0f);
    glVertex3d(-0.5, 0.5, z);

    glVertex3d(0.5, 0.5, z);

    glColor4f(r, g, b, 0.0f);
    glVertex3d(0.5, 0.5 + border, z);

    glVertex3d(-0.5, 0.5 + border, z);

    glEnd();

    glBegin(GL_QUADS);

    glColor4f(r, g, b, 1.0f);
    glVertex3d(-0.5, -0.5, z);

    glVertex3d(-0.5, 0.5, z);

    glColor4f(r, g, b, 0.0f);
    glVertex3d(-0.5 - border, 0.5, z);

    glVertex3d(-0.5 - border, -0.5, z);

    glEnd();

    glBegin(GL_QUADS);

    glColor4f(r, g, b, 1.0f);
    glVertex3d(0.5, -0.5, z);

    glVertex3d(0.5, 0.5, z);

    glColor4f(r, g, b, 0.0f);
    glVertex3d(0.5 + border, 0.5, z);

    glVertex3d(0.5 + border, -0.5, z);

    glEnd();

    glDisable(GL_BLEND);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glDisable(GL_BLEND);

    glColor4f(1, 1, 1, 1);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDepthMask(GL_TRUE);

    // мышка
    glPushMatrix();

    glTranslated(-3.35, -3.7, -2.645);
    glRotated(204, 0, 0, 1);
    glScaled(0.03, 0.03, 0.03);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    glColor3f(1, 1, 1);

    mouse.Bind();
    mouse_model.Draw();

    glPopMatrix();

    // клавиатура
    glPushMatrix();

    glTranslated(-3.3, -5.0, -2.63);

    glRotated(90, 0, 0, 1);

    glScaled(0.03, 0.03, 0.03);

    glEnable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    keyboard.Bind();
    glColor3f(1, 1, 1);
    keyboard_model.Draw();
    glPopMatrix();

    // кровать
    glPushMatrix();

    glTranslated(3.1, -6.2, -4.95);  

    glScaled(0.03, 0.03, 0.03);

    glRotated(0, 1, 0, 0);
    glRotated(180, 0, 0, 1);

    bed.Bind();
    bed_model.Draw();

    glPopMatrix();

    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, 0);

    // вентилятор
    double fx = -4.0;
    double fy = 7.0;
    double fz = -4.95;

    Shader::DontUseShaders();

    glPushMatrix();
    glTranslated(fx, fy, fz);

    // основание
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3d(0, 0, 0);
    for (int i = 0; i <= 360; i += 10) {
        double a = i * 3.14159 / 180;
        glVertex3d(cos(a) * 0.5, sin(a) * 0.5, 0);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex3d(0, 0, 0.08);
    for (int i = 0; i <= 360; i += 10) {
        double a = i * 3.14159 / 180;
        glVertex3d(cos(a) * 0.5, sin(a) * 0.5, 0.08);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 360; i += 10) {
        double a = i * 3.14159 / 180;
        double x = cos(a) * 0.5;
        double y = sin(a) * 0.5;
        glVertex3d(x, y, 0);
        glVertex3d(x, y, 0.08);
    }
    glEnd();

    // стойка
    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_QUADS);
    // передняя сторона
    glVertex3d(-0.07, -0.07, 0.08);
    glVertex3d(0.07, -0.07, 0.08);
    glVertex3d(0.07, 0.07, 1.8);
    glVertex3d(-0.07, 0.07, 1.8);
    // задняя сторона
    glVertex3d(-0.07, -0.07, 0.08);
    glVertex3d(-0.07, 0.07, 1.8);
    glVertex3d(0.07, 0.07, 1.8);
    glVertex3d(0.07, -0.07, 0.08);
    // левая сторона
    glVertex3d(-0.07, -0.07, 0.08);
    glVertex3d(-0.07, -0.07, 1.8);
    glVertex3d(-0.07, 0.07, 1.8);
    glVertex3d(-0.07, 0.07, 0.08);
    // правая сторона
    glVertex3d(0.07, -0.07, 0.08);
    glVertex3d(0.07, 0.07, 0.08);
    glVertex3d(0.07, 0.07, 1.8);
    glVertex3d(0.07, -0.07, 1.8);
    glEnd();

    // мотор
    glPushMatrix();
    glTranslated(0, 0, 1.8);

    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3d(-0.22, -0.22, -0.12);
    glVertex3d(0.22, -0.22, -0.12);
    glVertex3d(0.22, 0.22, 0.12);
    glVertex3d(-0.22, 0.22, 0.12);
    glEnd();

    // сетка
    glPushMatrix();
    glRotated(90, 1, 0, 0);

    glColor3f(0.5f, 0.5f, 0.5f);

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 60; i++) {
        float a = i * 2.0f * 3.14159f / 60.0f;
        glVertex3d(cos(a) * 0.9, sin(a) * 0.9, 0);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 60; i++) {
        float a = i * 2.0f * 3.14159f / 60.0f;
        glVertex3d(cos(a) * 0.35, sin(a) * 0.35, 0);
    }
    glEnd();

    for (int i = 0; i < 16; i++) {
        float a = i * 2.0f * 3.14159f / 16.0f;
        glBegin(GL_LINES);
        glVertex3d(cos(a) * 0.35, sin(a) * 0.35, 0);
        glVertex3d(cos(a) * 0.9, sin(a) * 0.9, 0);
        glEnd();
    }

    for (float r = 0.5; r <= 0.85; r += 0.17) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 60; i++) {
            float a = i * 2.0f * 3.14159f / 60.0f;
            glVertex3d(cos(a) * r, sin(a) * r, 0);
        }
        glEnd();
    }

    glPopMatrix();

    // лопасти
    glPushMatrix();
    glRotated(90, 1, 0, 0);
    glRotated(fanAngle, 0, 0, 1);

    int blades = 5;

    for (int i = 0; i < blades; i++)
    {
        glPushMatrix();
        glRotated(i * (360.0 / blades), 0, 0, 1);

        glColor3f(0.88f, 0.88f, 0.88f);

        glBegin(GL_TRIANGLE_STRIP);

        for (int j = 0; j <= 30; j++)
        {
            float t = j / 30.0f;

            float angle = t * 0.55f;

            float r1 = 0.18f + t * 0.55f;
            float r2 = r1 + 0.22f;

            float curve = sin(t * 3.1415f) * 0.08f;

            float x1 = cos(angle) * r1;
            float y1 = sin(angle) * r1 + curve;

            float x2 = cos(angle) * r2;
            float y2 = sin(angle) * r2 + curve;

            glVertex3d(x1, y1, -0.05);
            glVertex3d(x2, y2, 0.05);
        }

        glEnd();

        glPopMatrix();
    }

    glPopMatrix();

    glPopMatrix(); 

    glPopMatrix(); 

    // дверь

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    glDisable(GL_BLEND);

    glColor4f(1, 1, 1, 1);

    door.Bind();

    double doorWidth = 7.0;
    double doorHeight = 10.0;

    double eps = -0.02;

    double y = front + eps;

    double x1 = -doorWidth / 2.0;
    double x2 = doorWidth / 2.0;

    double z1 = -5.5;
    double z2 = z1 + doorHeight;

    glBegin(GL_QUADS);
    glNormal3d(0, -1, 0);

    glTexCoord2d(0, 0);
    glVertex3d(x1, y, z1);

    glTexCoord2d(1, 0);
    glVertex3d(x2, y, z1);

    glTexCoord2d(1, 1);
    glVertex3d(x2, y, z2);

    glTexCoord2d(0, 1);
    glVertex3d(x1, y, z2);

    glEnd();

    glDisable(GL_ALPHA_TEST);

    // плинтусы
    float plinthHeight = 0.18f;
    float plinthThickness = 0.04f;

    plintus.Bind();

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);

    glNormal3d(0, 1, 0);

    glTexCoord2d(0, 0);
    glVertex3d(left, back + plinthThickness, bottom);

    glTexCoord2d(6, 0);
    glVertex3d(right, back + plinthThickness, bottom);

    glTexCoord2d(6, 1);
    glVertex3d(right, back + plinthThickness, bottom + plinthHeight);

    glTexCoord2d(0, 1);
    glVertex3d(left, back + plinthThickness, bottom + plinthHeight);

    glEnd();

    glBegin(GL_QUADS);

    glNormal3d(0, -1, 0);

    glTexCoord2d(0, 0);
    glVertex3d(left, front - plinthThickness, bottom);

    glTexCoord2d(6, 0);
    glVertex3d(right, front - plinthThickness, bottom);

    glTexCoord2d(6, 1);
    glVertex3d(right, front - plinthThickness, bottom + plinthHeight);

    glTexCoord2d(0, 1);
    glVertex3d(left, front - plinthThickness, bottom + plinthHeight);

    glEnd();

    glBegin(GL_QUADS);

    glNormal3d(1, 0, 0);

    glTexCoord2d(0, 0);
    glVertex3d(left + plinthThickness, back, bottom);

    glTexCoord2d(8, 0);
    glVertex3d(left + plinthThickness, front, bottom);

    glTexCoord2d(8, 1);
    glVertex3d(left + plinthThickness, front, bottom + plinthHeight);

    glTexCoord2d(0, 1);
    glVertex3d(left + plinthThickness, back, bottom + plinthHeight);

    glEnd();

    glBegin(GL_QUADS);

    glNormal3d(-1, 0, 0);

    glTexCoord2d(0, 0);
    glVertex3d(right - plinthThickness, front, bottom);

    glTexCoord2d(8, 0);
    glVertex3d(right - plinthThickness, back, bottom);

    glTexCoord2d(8, 1);
    glVertex3d(right - plinthThickness, back, bottom + plinthHeight);

    glTexCoord2d(0, 1);
    glVertex3d(right - plinthThickness, front, bottom + plinthHeight);

    glEnd();

    // лампа 

    Shader::DontUseShaders();

    glPushMatrix();

    glTranslated(0, 0, top - 0.7);
    glScaled(0.015, 0.015, 0.015);
    glRotated(90, 0, 0, 1);

    lamp.Bind();

    lamp_model.Draw();

    glPopMatrix();

    glEnable(GL_TEXTURE_2D);



    //===============================================

    // Сбрасываем все трансформации
    glLoadIdentity();
    camera.SetUpCamera();
    Shader::DontUseShaders();
    // Рисуем источник света
    light.DrawLightGizmo();

    //================Сообщение в верхнем левом углу=======================
    glActiveTexture(GL_TEXTURE0);
    // Переключаемся на матрицу проекции
    glMatrixMode(GL_PROJECTION);
    // Сохраняем текущую матрицу проекции с перспективным преобразованием
    glPushMatrix();
    // Загружаем единичную матрицу в матрицу проекции
    glLoadIdentity();

    // Устанавливаем матрицу параллельной проекции
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

    // Переключаемся на матрицу MODELVIEW
    glMatrixMode(GL_MODELVIEW);
    // Сохраняем матрицу
    glPushMatrix();
    // Сбрасываем все трансформации и настройки камеры загрузкой единичной матрицы
    glLoadIdentity();

    // Нарисованное тут находится в 2D системе координат
    // Нижний левый угол окна - точка (0,0)
    // Верхний правый угол (ширина_окна - 1, высота_окна - 1)


    // ========== ВРЕМЕННОЕ УПРАВЛЕНИЕ КАМЕРОЙ (WASD + мышь) ==========
    static float freeX = 0, freeY = 2, freeZ = 5;
    static float angleX = 0, angleY = 0;
    static bool wasdMode = true;  // всегда включено

    float speed = 3.0f * delta_time;

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3) << "T - " << (texturing ? L"[вкл]выкл" : L"вкл[выкл]") << L" текстур\n"
        << "L - " << (lightning ? L"[вкл]выкл" : L"вкл[выкл]") << L" освещение\n"
        << "A - " << (alpha ? L"[вкл]выкл" : L"вкл[выкл]") << L" альфа-наложение\n"
        << L"F - переместить свет в позицию камеры\n"
        << L"G - двигать свет по горизонтали\n"
        << L"G+ЛКМ - двигать свет по вертикали\n"
        << L"1 - вкл/выкл компьютер\n"
        << L"Координаты света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7)
        << light.z() << ")\n"
        << L"Координаты камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << ","
        << std::setw(7) << camera.z() << ")\n"
        << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ", fi1=" << std::setw(7) << camera.fi1()
        << ", fi2=" << std::setw(7) << camera.fi2() << '\n'
        << L"delta_time: " << std::setprecision(5) << delta_time << '\n'
        << L"full_time: " << std::setprecision(2) << full_time << std::endl;

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    // Восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
