#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QPushButton *loadButton;
    QPushButton *encodeButton;
    QPushButton *decodeButton;
    QTableWidget *table;

    QString decodeFileName;

    QString fileExtention;



    QByteArray fileContents;
    QVector <int> frequencies;
    int fileSize;
    int bitsToDecode;


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();



public slots:
    void loadButtonClicked();
    void encodeButtonClicked();
    void decodeButtonClicked();



};
#endif // MAINWINDOW_H
