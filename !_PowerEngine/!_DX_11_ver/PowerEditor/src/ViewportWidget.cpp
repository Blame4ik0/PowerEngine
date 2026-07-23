#include "ViewportWidget.h"
#include "MeshBoundsCache.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

// ============================================================
//  Shaders
// ============================================================

// Wireframe — position only, uniform color + MVP
static const char* kWireVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kWireFS = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() { FragColor = vec4(uColor, 1.0); }
)";

// Diffuse mesh — position + normal (interleaved, 6 floats per vertex)
// Simple half-Lambert with a fixed sun direction, no textures.
// Selected entities get a tint overlay.
static const char* kMeshVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4  uMVP;
uniform mat3  uNormalMat;
out vec3 vNormal;
void main()
{
    vNormal = normalize(uNormalMat * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kMeshFS = R"(
#version 330 core
in vec3 vNormal;
uniform vec3  uEntityColor;  // flat tint per entity
uniform vec3  uLightDir;     // normalized, world space
uniform bool  uSelected;
out vec4 FragColor;
void main()
{
    float NdotL   = max(dot(normalize(vNormal), uLightDir), 0.0);
    float diffuse = 0.2 + 0.8 * NdotL; // ambient 0.2
    vec3 color = uEntityColor * diffuse;
    if (uSelected) color = mix(color, vec3(1.0, 0.65, 0.1), 0.35);
    FragColor = vec4(color, 1.0);
}
)";

// ============================================================
//  Construction / destruction
// ============================================================

ViewportWidget::ViewportWidget(SceneDocument* doc, QWidget* parent)
    : QOpenGLWidget(parent)
    , m_doc(doc)
{
    setFocusPolicy(Qt::StrongFocus);
    connect(m_doc, &SceneDocument::SceneChanged,
            this, &ViewportWidget::OnSceneChanged);
    connect(m_doc, &SceneDocument::EntityModified,
            this, [this](int) { update(); });
}

ViewportWidget::~ViewportWidget()
{
    makeCurrent();
    qDeleteAll(m_meshCache);
    m_meshCache.clear();
    doneCurrent();
}

// ============================================================
//  initializeGL
// ============================================================

void ViewportWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // ---- Wireframe shader ----
    m_wireProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,   kWireVS);
    m_wireProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kWireFS);
    m_wireProgram.link();
    m_wireMvpLoc   = m_wireProgram.uniformLocation("uMVP");
    m_wireColorLoc = m_wireProgram.uniformLocation("uColor");

    // ---- Mesh diffuse shader ----
    m_meshProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,   kMeshVS);
    m_meshProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFS);
    m_meshProgram.link();
    m_meshMvpLoc       = m_meshProgram.uniformLocation("uMVP");
    m_meshNormalMatLoc = m_meshProgram.uniformLocation("uNormalMat");
    m_meshColorLoc     = m_meshProgram.uniformLocation("uEntityColor");
    m_meshLightDirLoc  = m_meshProgram.uniformLocation("uLightDir");
    m_meshSelectedLoc  = m_meshProgram.uniformLocation("uSelected");

    // ---- Grid ----
    std::vector<float> grid;
    for (int i = -15; i <= 15; i++)
    {
        float t = static_cast<float>(i);
        grid.insert(grid.end(), { -15.f, 0.f, t,  15.f, 0.f, t });
        grid.insert(grid.end(), { t, 0.f, -15.f,  t, 0.f, 15.f });
    }
    m_gridVertexCount = static_cast<int>(grid.size() / 3);
    m_gridVbo.create(); m_gridVbo.bind();
    m_gridVbo.allocate(grid.data(), static_cast<int>(grid.size() * sizeof(float)));
    m_gridVbo.release();

    BuildCubeLines();
    BuildSphereLines();
    BuildPlaneMesh();
}

void ViewportWidget::BuildCubeLines()
{
    constexpr float h = 0.5f;
    std::vector<float> v;
    auto L = [&](float x0,float y0,float z0, float x1,float y1,float z1){
        v.insert(v.end(), {x0,y0,z0, x1,y1,z1});
    };
    L(-h,-h,-h, h,-h,-h); L( h,-h,-h, h,-h, h);
    L( h,-h, h,-h,-h, h); L(-h,-h, h,-h,-h,-h);
    L(-h, h,-h, h, h,-h); L( h, h,-h, h, h, h);
    L( h, h, h,-h, h, h); L(-h, h, h,-h, h,-h);
    L(-h,-h,-h,-h, h,-h); L( h,-h,-h, h, h,-h);
    L( h,-h, h, h, h, h); L(-h,-h, h,-h, h, h);
    m_cubeVertexCount = static_cast<int>(v.size() / 3);
    m_cubeVbo.create(); m_cubeVbo.bind();
    m_cubeVbo.allocate(v.data(), static_cast<int>(v.size() * sizeof(float)));
    m_cubeVbo.release();
}

void ViewportWidget::BuildSphereLines()
{
    // Three circles (XY, XZ, YZ planes) at radius 1
    std::vector<float> v;
    constexpr int kSegs = 32;
    for (int c = 0; c < 3; c++)
    for (int i = 0; i < kSegs; i++)
    {
        float a0 = static_cast<float>(i)     / kSegs * 2.f * M_PI;
        float a1 = static_cast<float>(i + 1) / kSegs * 2.f * M_PI;
        float c0 = cosf(a0), s0 = sinf(a0);
        float c1 = cosf(a1), s1 = sinf(a1);
        if (c == 0) { v.insert(v.end(), {c0,s0,0, c1,s1,0}); } // XY
        if (c == 1) { v.insert(v.end(), {c0,0,s0, c1,0,s1}); } // XZ
        if (c == 2) { v.insert(v.end(), {0,c0,s0, 0,c1,s1}); } // YZ
    }
    m_sphereVertexCount = static_cast<int>(v.size() / 3);
    m_sphereVbo.create(); m_sphereVbo.bind();
    m_sphereVbo.allocate(v.data(), static_cast<int>(v.size() * sizeof(float)));
    m_sphereVbo.release();
}

void ViewportWidget::BuildPlaneMesh()
{
    // Unit quad on the XZ plane (normal = +Y), centered at origin.
    // Entities of meshSource.type == "plane" scale this by their actual
    // width/height instead of drawing a wireframe box, so the floor
    // (and any other plane) reads as a filled surface like the real
    // engine renders it, not an abstract cube gizmo.
    // Position(3) + Normal(3) per vertex, matching the mesh shader's
    // vertex layout so DrawEntityMesh's diffuse shader can be reused.
    constexpr float h = 0.5f;
    std::vector<float> v = {
        -h, 0.f, -h,   0.f, 1.f, 0.f,
        -h, 0.f,  h,   0.f, 1.f, 0.f,
         h, 0.f,  h,   0.f, 1.f, 0.f,

        -h, 0.f, -h,   0.f, 1.f, 0.f,
         h, 0.f,  h,   0.f, 1.f, 0.f,
         h, 0.f, -h,   0.f, 1.f, 0.f,
    };
    m_planeVertexCount = static_cast<int>(v.size() / 6);
    m_planeVbo.create(); m_planeVbo.bind();
    m_planeVbo.allocate(v.data(), static_cast<int>(v.size() * sizeof(float)));
    m_planeVbo.release();
}

// ============================================================
//  Mesh GPU upload
// ============================================================

ViewportWidget::MeshGPU* ViewportWidget::GetOrUploadMesh(const QString& filepath)
{
    auto it = m_meshCache.find(filepath);
    if (it != m_meshCache.end()) return it.value();

    const auto& bounds = MeshBoundsCache::Get(filepath);
    auto* gpu = new MeshGPU();

    if (bounds.Valid && bounds.VertexCount > 0)
    {
        gpu->vbo.create();
        gpu->vbo.bind();
        gpu->vbo.allocate(bounds.VertexData.data(),
            static_cast<int>(bounds.VertexData.size() * sizeof(float)));
        gpu->vbo.release();
        gpu->vertexCount = bounds.VertexCount;
        gpu->ready = true;
    }

    m_meshCache.insert(filepath, gpu);
    return gpu;
}

// ============================================================
//  Camera
// ============================================================

QMatrix4x4 ViewportWidget::BuildViewMatrix() const
{
    float yr = qDegreesToRadians(m_yaw);
    float pr = qDegreesToRadians(m_pitch);
    QVector3D offset(
        m_distance * cosf(pr) * sinf(yr),
        m_distance * sinf(pr),
        m_distance * cosf(pr) * cosf(yr));
    QMatrix4x4 v;
    v.lookAt(m_target + offset, m_target, {0,1,0});
    return v;
}

// ============================================================
//  Paint
// ============================================================

void ViewportWidget::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void ViewportWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 proj;
    float aspect = width() > 0 && height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height()) : 1.f;
    proj.perspective(50.f, aspect, 0.05f, 2000.f);
    QMatrix4x4 view = BuildViewMatrix();
    QMatrix4x4 vp   = proj * view;

    DrawGrid(vp);
    DrawEntities(view, proj);
    DrawLightSpheres(vp);
}

void ViewportWidget::DrawGrid(const QMatrix4x4& vp)
{
    m_wireProgram.bind();
    m_wireProgram.setUniformValue(m_wireMvpLoc,   vp);
    m_wireProgram.setUniformValue(m_wireColorLoc, QVector3D(0.3f, 0.3f, 0.32f));
    m_gridVbo.bind();
    m_wireProgram.enableAttributeArray(0);
    m_wireProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3);
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVbo.release();
    m_wireProgram.release();
}

// Per-entity colors (cycles through a small palette so entities are distinct)
static QVector3D EntityColor(int index)
{
    static const QVector3D palette[] = {
        {0.55f,0.75f,1.00f}, {1.00f,0.70f,0.45f}, {0.55f,1.00f,0.65f},
        {1.00f,0.55f,0.70f}, {0.80f,0.55f,1.00f}, {1.00f,1.00f,0.55f},
    };
    return palette[index % 6];
}

void ViewportWidget::DrawEntities(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    for (int i = 0; i < m_doc->EntityCount(); i++)
    {
        const auto& e = m_doc->EntityAt(i);
        if (!e.contains("transform")) continue;

        // Skip pure light entities (no mesh) — they're drawn as sphere gizmos
        if (!e.contains("meshSource") && !e.contains("material")) continue;
        if (e.contains("pointLight") || e.contains("directionalLight")) continue;

        const auto& t   = e["transform"];
        const auto& pos = t.value("position", nlohmann::json::array({0,0,0}));
        const auto& rot = t.value("rotation", nlohmann::json::array({0,0,0}));
        const auto& scl = t.value("scale",    nlohmann::json::array({1,1,1}));

        QMatrix4x4 model;
        model.translate(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
        // Must reproduce the engine's XMMatrixRotationRollPitchYaw exactly:
        // for a row-vector v, v' = v * Rz(roll) * Rx(pitch) * Ry(yaw) —
        // i.e. roll is applied to the vector FIRST, pitch second, yaw LAST.
        // Qt's QMatrix4x4::rotate() post-multiplies the current matrix
        // (M = M * R), and vectors transform as v' = M * v (column-vector),
        // so the call issued FIRST ends up applied to the vector LAST.
        // To get roll-first/pitch-second/yaw-last, call order must be:
        // yaw, then pitch, then roll.
        model.rotate(rot[1].get<float>(), 0, 1, 0); // yaw   (Y)
        model.rotate(rot[0].get<float>(), 1, 0, 0); // pitch (X)
        model.rotate(rot[2].get<float>(), 0, 0, 1); // roll  (Z)
        model.scale(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());

        bool selected = (i == m_selectedIndex);

        // Plane primitive → draw as an actual filled quad, not a wireframe box
        if (e.contains("meshSource") && e["meshSource"].value("type","") == "plane")
        {
            float w = e["meshSource"].value("width",  1.0f);
            float h = e["meshSource"].value("height", 1.0f);
            DrawEntityPlane(i, model, view, proj, selected, w, h);
            continue;
        }

        // File-based mesh → try to draw actual geometry
        if (e.contains("meshSource") && e["meshSource"].value("type","") == "file")
        {
            QString path = QString::fromStdString(
                e["meshSource"].value("filepath", std::string()));
            if (!path.isEmpty())
            {
                MeshGPU* gpu = GetOrUploadMesh(path);
                if (gpu && gpu->ready)
                {
                    DrawEntityMesh(i, model, view, proj, selected);
                    continue;
                }
            }
        }

        // Fallback: draw wireframe AABB box
        QMatrix4x4 vp = proj * view;
        if (e.contains("meshSource") && e["meshSource"].value("type","") == "file")
        {
            QString path = QString::fromStdString(
                e["meshSource"].value("filepath", std::string()));
            const auto& b = MeshBoundsCache::Get(path);
            if (b.Valid)
            {
                QMatrix4x4 boxModel = model;
                QVector3D center = (b.Min + b.Max) * 0.5f;
                QVector3D size   =  b.Max - b.Min;
                boxModel.translate(center);
                boxModel.scale(size.x(), size.y(), size.z());
                DrawEntityBox(boxModel, vp, selected);
                continue;
            }
        }
        DrawEntityBox(model, vp, selected);
    }
}

void ViewportWidget::DrawEntityMesh(int index, const QMatrix4x4& model,
                                     const QMatrix4x4& view, const QMatrix4x4& proj,
                                     bool selected)
{
    QString path = QString::fromStdString(
        m_doc->EntityAt(index)["meshSource"].value("filepath", std::string()));
    MeshGPU* gpu = m_meshCache.value(path, nullptr);
    if (!gpu || !gpu->ready) return;

    QMatrix4x4 mvp = proj * view * model;
    QMatrix3x3 normalMat = model.normalMatrix();

    static const QVector3D kLightDir = QVector3D(0.4f, 1.f, 0.6f).normalized();

    m_meshProgram.bind();
    m_meshProgram.setUniformValue(m_meshMvpLoc,       mvp);
    m_meshProgram.setUniformValue(m_meshNormalMatLoc, normalMat);
    m_meshProgram.setUniformValue(m_meshColorLoc,     EntityColor(index));
    m_meshProgram.setUniformValue(m_meshLightDirLoc,  kLightDir);
    m_meshProgram.setUniformValue(m_meshSelectedLoc,  selected);

    gpu->vbo.bind();
    // stride = 6 floats (3 pos + 3 normal), 12 bytes = sizeof(float)*3
    m_meshProgram.enableAttributeArray(0);
    m_meshProgram.setAttributeBuffer(0, GL_FLOAT, 0,            3, 6 * sizeof(float));
    m_meshProgram.enableAttributeArray(1);
    m_meshProgram.setAttributeBuffer(1, GL_FLOAT, 3*sizeof(float), 3, 6 * sizeof(float));

    glDrawArrays(GL_TRIANGLES, 0, gpu->vertexCount);
    gpu->vbo.release();
    m_meshProgram.release();

    // Draw a thin wireframe AABB outline on top of selected entities
    if (selected)
    {
        const auto& b = MeshBoundsCache::Get(path);
        if (b.Valid)
        {
            QMatrix4x4 boxModel = model;
            QVector3D center = (b.Min + b.Max) * 0.5f;
            QVector3D size   =  b.Max - b.Min;
            boxModel.translate(center);
            boxModel.scale(size.x(), size.y(), size.z());
            DrawEntityBox(boxModel, proj * view, true);
        }
    }
}

void ViewportWidget::DrawEntityPlane(int index, const QMatrix4x4& model,
                                      const QMatrix4x4& view, const QMatrix4x4& proj,
                                      bool selected, float width, float height)
{
    // Scale the unit quad (built in BuildPlaneMesh) to the plane's actual
    // authored dimensions, then apply the entity's own transform on top —
    // same order as everything else, so position/rotation still work
    // normally on planes (e.g. a tilted platform, not just a flat floor).
    QMatrix4x4 planeModel = model;
    planeModel.scale(width, 1.0f, height);

    QMatrix4x4 mvp = proj * view * planeModel;
    QMatrix3x3 normalMat = planeModel.normalMatrix();

    static const QVector3D kLightDir = QVector3D(0.4f, 1.f, 0.6f).normalized();

    m_meshProgram.bind();
    m_meshProgram.setUniformValue(m_meshMvpLoc,       mvp);
    m_meshProgram.setUniformValue(m_meshNormalMatLoc, normalMat);
    m_meshProgram.setUniformValue(m_meshColorLoc,     EntityColor(index));
    m_meshProgram.setUniformValue(m_meshLightDirLoc,  kLightDir);
    m_meshProgram.setUniformValue(m_meshSelectedLoc,  selected);

    m_planeVbo.bind();
    m_meshProgram.enableAttributeArray(0);
    m_meshProgram.setAttributeBuffer(0, GL_FLOAT, 0,              3, 6 * sizeof(float));
    m_meshProgram.enableAttributeArray(1);
    m_meshProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

    glDrawArrays(GL_TRIANGLES, 0, m_planeVertexCount);
    m_planeVbo.release();
    m_meshProgram.release();
}

void ViewportWidget::DrawEntityBox(const QMatrix4x4& model, const QMatrix4x4& vp,
                                    bool selected)
{
    m_wireProgram.bind();
    m_wireProgram.setUniformValue(m_wireMvpLoc, vp * model);
    m_wireProgram.setUniformValue(m_wireColorLoc,
        selected ? QVector3D(1.f, 0.65f, 0.1f) : QVector3D(0.4f, 0.75f, 1.f));
    m_cubeVbo.bind();
    m_wireProgram.enableAttributeArray(0);
    m_wireProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3);
    glDrawArrays(GL_LINES, 0, m_cubeVertexCount);
    m_cubeVbo.release();
    m_wireProgram.release();
}

void ViewportWidget::DrawLightSpheres(const QMatrix4x4& vp)
{
    m_wireProgram.bind();

    for (int i = 0; i < m_doc->EntityCount(); i++)
    {
        const auto& e = m_doc->EntityAt(i);
        if (!e.contains("pointLight")) continue;

        const auto& pl  = e["pointLight"];
        const auto& pos = pl.value("position", nlohmann::json::array({0,0,0}));
        float radius    = pl.value("radius", 10.f);
        const auto& col = pl.value("color",  nlohmann::json::array({1,1,1}));
        QVector3D lightColor(col[0].get<float>(), col[1].get<float>(), col[2].get<float>());

        QVector3D pos3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());

        // Always draw a small fixed-size box marking the light's position,
        // so lights remain visible/findable at any zoom level regardless
        // of their (potentially huge) radius.
        {
            QMatrix4x4 markerModel;
            markerModel.translate(pos3);
            markerModel.scale(0.3f); // fixed small size, independent of radius
            m_wireProgram.setUniformValue(m_wireMvpLoc,   vp * markerModel);
            m_wireProgram.setUniformValue(m_wireColorLoc, lightColor);
            m_cubeVbo.bind();
            m_wireProgram.enableAttributeArray(0);
            m_wireProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3);
            glDrawArrays(GL_LINES, 0, m_cubeVertexCount);
            m_cubeVbo.release();
        }

        // Only draw the radius sphere for the currently selected light —
        // otherwise scenes with many point lights become an unreadable
        // mess of overlapping wireframe spheres.
        if (i == m_selectedIndex)
        {
            QMatrix4x4 sphereModel;
            sphereModel.translate(pos3);
            sphereModel.scale(radius);
            m_wireProgram.setUniformValue(m_wireMvpLoc,   vp * sphereModel);
            m_wireProgram.setUniformValue(m_wireColorLoc, lightColor);
            m_sphereVbo.bind();
            m_wireProgram.enableAttributeArray(0);
            m_wireProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3);
            glDrawArrays(GL_LINES, 0, m_sphereVertexCount);
            m_sphereVbo.release();
        }
    }

    m_wireProgram.release();
}

// ============================================================
//  Picking — AABB screen-space test
// ============================================================

void ViewportWidget::PickEntityAt(const QPoint& screenPos)
{
    QMatrix4x4 proj;
    float aspect = width() > 0 && height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height()) : 1.f;
    proj.perspective(50.f, aspect, 0.05f, 2000.f);
    QMatrix4x4 view   = BuildViewMatrix();
    QMatrix4x4 viewProj = proj * view;

    int   bestIndex = -1;
    float bestArea  = 0.f; // pick the entity with largest projected screen area

    for (int i = 0; i < m_doc->EntityCount(); i++)
    {
        const auto& e = m_doc->EntityAt(i);
        if (!e.contains("transform")) continue;
        if (e.contains("pointLight") || e.contains("directionalLight")) continue;

        const auto& t   = e["transform"];
        const auto& pos = t.value("position", nlohmann::json::array({0,0,0}));
        const auto& rot = t.value("rotation", nlohmann::json::array({0,0,0}));
        const auto& scl = t.value("scale",    nlohmann::json::array({1,1,1}));

        QMatrix4x4 model;
        model.translate(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
        // Must reproduce the engine's XMMatrixRotationRollPitchYaw exactly:
        // for a row-vector v, v' = v * Rz(roll) * Rx(pitch) * Ry(yaw) —
        // i.e. roll is applied to the vector FIRST, pitch second, yaw LAST.
        // Qt's QMatrix4x4::rotate() post-multiplies the current matrix
        // (M = M * R), and vectors transform as v' = M * v (column-vector),
        // so the call issued FIRST ends up applied to the vector LAST.
        // To get roll-first/pitch-second/yaw-last, call order must be:
        // yaw, then pitch, then roll.
        model.rotate(rot[1].get<float>(), 0, 1, 0); // yaw   (Y)
        model.rotate(rot[0].get<float>(), 1, 0, 0); // pitch (X)
        model.rotate(rot[2].get<float>(), 0, 0, 1); // roll  (Z)
        model.scale(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());

        // Get the 8 AABB corners in world space
        MeshBoundsCache::Bounds bounds;
        if (e.contains("meshSource") && e["meshSource"].value("type","") == "file")
        {
            QString path = QString::fromStdString(
                e["meshSource"].value("filepath", std::string()));
            bounds = MeshBoundsCache::Get(path);
        }
        if (!bounds.Valid)
        {
            // use a small default box around the entity's origin
            bounds.Min = {-0.5f,-0.5f,-0.5f};
            bounds.Max = { 0.5f, 0.5f, 0.5f};
        }

        QVector3D corners[8] = {
            {bounds.Min.x(),bounds.Min.y(),bounds.Min.z()},
            {bounds.Max.x(),bounds.Min.y(),bounds.Min.z()},
            {bounds.Min.x(),bounds.Max.y(),bounds.Min.z()},
            {bounds.Max.x(),bounds.Max.y(),bounds.Min.z()},
            {bounds.Min.x(),bounds.Min.y(),bounds.Max.z()},
            {bounds.Max.x(),bounds.Min.y(),bounds.Max.z()},
            {bounds.Min.x(),bounds.Max.y(),bounds.Max.z()},
            {bounds.Max.x(),bounds.Max.y(),bounds.Max.z()},
        };

        // Project all 8 corners to screen space, compute 2D bounding rect
        float sxMin = 1e9f, sxMax = -1e9f;
        float syMin = 1e9f, syMax = -1e9f;
        bool anyInFront = false;

        for (auto& c : corners)
        {
            QVector4D clip = viewProj * (model * QVector4D(c, 1.f));
            if (clip.w() <= 0.f) continue;
            anyInFront = true;
            float sx = (clip.x()/clip.w() * 0.5f + 0.5f) * width();
            float sy = (1.f - (clip.y()/clip.w() * 0.5f + 0.5f)) * height();
            sxMin = std::min(sxMin, sx); sxMax = std::max(sxMax, sx);
            syMin = std::min(syMin, sy); syMax = std::max(syMax, sy);
        }

        if (!anyInFront) continue;

        // Test if click is inside the projected rect
        float px = static_cast<float>(screenPos.x());
        float py = static_cast<float>(screenPos.y());
        if (px < sxMin || px > sxMax || py < syMin || py > syMax) continue;

        // Among overlapping entities, prefer the one with smaller projected area
        // (i.e. furthest, or smallest — avoids always picking the floor)
        float area = (sxMax - sxMin) * (syMax - syMin);
        if (bestIndex == -1 || area < bestArea)
        {
            bestArea  = area;
            bestIndex = i;
        }
    }

    m_selectedIndex = bestIndex;
    update();
    emit EntityPicked(bestIndex);
}

// ============================================================
//  Input
// ============================================================

void ViewportWidget::SetSelectedEntity(int index)
{
    m_selectedIndex = index;
    update();
}

void ViewportWidget::OnSceneChanged()
{
    update();
}

void ViewportWidget::mousePressEvent(QMouseEvent* e)
{
    m_lastMousePos = e->pos();
    m_orbiting = false;
    m_panning  = false;
    if (e->button() == Qt::RightButton)  m_orbiting = true;
    if (e->button() == Qt::MiddleButton) m_panning  = true;
    setFocus();
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* e)
{
    QPoint delta = e->pos() - m_lastMousePos;
    m_lastMousePos = e->pos();

    if (m_orbiting)
    {
        m_yaw   += delta.x() * 0.4f;
        m_pitch += delta.y() * 0.4f;
        m_pitch = std::max(-89.f, std::min(89.f, m_pitch));
        update();
    }
    else if (m_panning)
    {
        float yr = qDegreesToRadians(m_yaw);
        QVector3D right( cosf(yr), 0, -sinf(yr));
        QVector3D up(0, 1, 0);
        float s = m_distance * 0.0015f;
        m_target -= right * delta.x() * s;
        m_target += up    * delta.y() * s;
        update();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)   PickEntityAt(e->pos());
    if (e->button() == Qt::RightButton)  m_orbiting = false;
    if (e->button() == Qt::MiddleButton) m_panning  = false;
}

void ViewportWidget::wheelEvent(QWheelEvent* e)
{
    float d = e->angleDelta().y() / 120.f;
    m_distance *= (1.f - d * 0.1f);
    m_distance = std::max(0.5f, std::min(500.f, m_distance));
    update();
}