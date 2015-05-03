#if 0
/**utilities 
 * main.cpp
 * */
#include <QGuiApplication>  
#include <QQuickView>

#include "Scene.h"
#include "Planet.h"
#include "Terrain.h"
#include "Mangekyou.h"
#include "nexus.h"

int main(int argc, char *argv[])  
{  
	QGuiApplication app(argc, argv);
	qmlRegisterType<Mangekyou>("Mangekyou", 1, 0, "Renderer");

	int ret;
	{
		QQuickView view;
		view.setFormat(nexus::select_gl(view.format()));
		view.setPersistentOpenGLContext(true);
		view.setPersistentSceneGraph(true);
		view.setResizeMode(QQuickView::SizeRootObjectToView);
		view.setSource(QUrl("qrc:///main.qml"));
		view.show();
		ret = app.exec();
	} // After this view will be destructed
	nexus::terminate();
	return ret;
}
#endif

#include <Application.h>
#include <RenderManager.h>

#include <Scene.h>
#include <Terrain.h>
#include <Ocean.h>
#include <TextureSkyDome.h>
#include <Grass.h>

class PlanetRenderManager : public RenderManager
{
public:

    /**
     * Notice: do not put your object instantiation code
     * in the construction. This is due to the fact that
     * when the constructor is called, the OpenGL context
     * has not been created, therefore the instantiation
     * will fail. Instead, please move those codes into 
     * prepare method.
     * */
    PlanetRenderManager() : resolution(1024, 768) {
    }

    virtual ~PlanetRenderManager() {

    }

    void keyPressed(int count, int key, int modifiers, QString text) {
        switch (key) {
            case Qt::Key_Left:
                camera_->turnLeft(5);
                break;
            case Qt::Key_Right:
                camera_->turnRight(5);
                break;
            case Qt::Key_Up:
                camera_->moveForward(50);
                break;
            case Qt::Key_Down:
                camera_->moveBackward(50);
                break;
            default:
                qDebug() << "Not arrow keys";
        }
    }

    void prepare() {
        int width = resolution.width();
        int height = resolution.height();

        // Create camera
        camera_.reset(new Camera("Viewer Camera"));
        camera_->setPerspective(45.0, (float)width/(float)height, 0.01, 5000.0);
        camera_->lookAt(
                QVector3D(0.0, 150.0, 0.0),
                QVector3D(0.0, 150.0, 20.0),
                QVector3D(0.0, 1.0, 0.0)
        );
        reflectCamera_.reset(new Camera("Reflection Camera"));

        // Create two passes
        reflection_.reset(new Scene("reflection", resolution));
        
        finalPass_.reset(new Scene("final result", resolution));
        finalPass_->setCamera(camera_.get());

        // Instantiate scene objects
        skydome_.reset(new TextureSkyDome(64, finalPass_.get()));
        terrain_.reset(new Terrain(64, 15, finalPass_.get()));
        ocean_.reset(new Ocean(64, 15, finalPass_.get()));
        // grass_.reset(new Grass(0x12345678));

        // Adding objects into reflection pass
        reflection_->addObject(skydome_.get());
        reflection_->addObject(terrain_.get());

        // Adding objects into final pass
        finalPass_->addObject(skydome_.get());
        finalPass_->addObject(terrain_.get());
        // finalPass_->addObject(ocean_.get());

        // OperGL settings
        glEnable(GL_DEPTH_TEST);
    }

    void render(QOpenGLFramebufferObject *fbo) {
        /**
         * Currently there seems to be an efficiency
         * issue with ocean rendering. That is why I
         * disable ocean at this point. I may refine
         * on the ocean later.
         * */
        qDebug() << "FPS:" << fps();

        // camera_->moveForward(500 * getInterval());
        // camera_->turnLeft(0.1);

        // reflection_->setCamera(camera_.get());
        // reflection_->renderScene();

        // finalPass_->useTexture(reflection_->texture());
        finalPass_->setCamera(camera_.get());
        finalPass_->renderScene(fbo);
    }

    void shutdown() {
        // Reset scenes
        reflection_.reset();
        finalPass_.reset();

        // Reset scene objects
        skydome_.reset();
        terrain_.reset();
        
        camera_.reset();
        reflectCamera_.reset();
    }

private:
    QSize resolution;

    // Render Targets
    std::unique_ptr<Scene> reflection_;
    // std::unique_ptr<Scene> shadow_;
    std::unique_ptr<Scene> finalPass_;

    // Scene Objects
    std::unique_ptr<Terrain> terrain_;
    std::unique_ptr<Ocean> ocean_;
    std::unique_ptr<TextureSkyDome> skydome_;
    std::unique_ptr<Grass> grass_;

    // Camera
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> reflectCamera_;
};


int main(int argc, char *argv[])
{
    /**
     * Sugguested framework:
     * 1. Create a Application wrapper, where common
     * stuff are managed, such as shaders containing common
     * functions, render managers.
     *
     * 2. Create a render manager, which is used to render
     * customized objects with render targets. The render
     * manager should be a singleton all over the application.
     * In the render manager, we can customize the rendering
     * sequence of multiple passes rendering.
     *
     * 3. Register the render manager in application so that 
     * the render thread will use render manager as the rendering
     * engine.
     * */

    Application app("PlanetRenderer", argc, argv);

    app.registerRenderManager(new PlanetRenderManager());

    return app.exec();
}
