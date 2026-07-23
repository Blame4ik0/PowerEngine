#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QHash>
#include "SceneDocument.h"

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit ViewportWidget(SceneDocument* doc, QWidget* parent = nullptr);
    ~ViewportWidget();

public slots:
    void SetSelectedEntity(int index);
    void OnSceneChanged();

signals:
    void EntityPicked(int index);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    // ---- Drawing ----
    void DrawGrid        (const QMatrix4x4& vp);
    void DrawEntities    (const QMatrix4x4& view, const QMatrix4x4& proj);
    void DrawEntityMesh  (int index, const QMatrix4x4& model,
                          const QMatrix4x4& view, const QMatrix4x4& proj,
                          bool selected);
    void DrawEntityBox   (const QMatrix4x4& model, const QMatrix4x4& vp,
                          bool selected);
    void DrawLightSpheres(const QMatrix4x4& vp);
    void PickEntityAt    (const QPoint& screenPos);
    QMatrix4x4 BuildViewMatrix() const;

    // ---- Mesh VBO cache (one per entity filepath) ----
    struct MeshGPU
    {
        QOpenGLBuffer vbo{ QOpenGLBuffer::VertexBuffer };
        int           vertexCount = 0;
        bool          ready       = false;
    };
    MeshGPU* GetOrUploadMesh(const QString& filepath);
    QHash<QString, MeshGPU*> m_meshCache;

    // ---- Geometry helpers ----
    void BuildCubeLines();
    void BuildSphereLines();
    void BuildPlaneMesh();
    void DrawEntityPlane(int index, const QMatrix4x4& model,
                        const QMatrix4x4& view, const QMatrix4x4& proj,
                        bool selected, float width, float height);

    SceneDocument* m_doc;
    int            m_selectedIndex = -1;

    // ---- Camera ----
    float     m_yaw      = 45.f;
    float     m_pitch    = 25.f;
    float     m_distance = 15.f;
    QVector3D m_target   = { 0, 0, 0 };
    QPoint    m_lastMousePos;
    bool      m_orbiting = false;
    bool      m_panning  = false;

    // ---- Wireframe shader (grid, AABB boxes, light spheres) ----
    QOpenGLShaderProgram m_wireProgram;
    int m_wireMvpLoc   = -1;
    int m_wireColorLoc = -1;

    // ---- Diffuse shader (actual mesh geometry) ----
    QOpenGLShaderProgram m_meshProgram;
    int m_meshMvpLoc      = -1;
    int m_meshNormalMatLoc = -1;
    int m_meshColorLoc    = -1;
    int m_meshLightDirLoc = -1;
    int m_meshSelectedLoc = -1;

    // ---- Static geometry VBOs ----
    QOpenGLBuffer m_gridVbo       { QOpenGLBuffer::VertexBuffer };
    QOpenGLBuffer m_cubeVbo       { QOpenGLBuffer::VertexBuffer };
    QOpenGLBuffer m_sphereVbo     { QOpenGLBuffer::VertexBuffer };
    QOpenGLBuffer m_planeVbo      { QOpenGLBuffer::VertexBuffer };
    int m_gridVertexCount   = 0;
    int m_cubeVertexCount   = 0;
    int m_sphereVertexCount = 0;
    int m_planeVertexCount  = 0;
};