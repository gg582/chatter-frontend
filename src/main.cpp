#include "MainWindow.h"
#include "CommandCatalog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<CommandDescriptor>("CommandDescriptor");

    MainWindow window;
    window.resize(900, 600);
    window.show();

    return app.exec();
}
