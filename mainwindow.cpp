#include "mainwindow.h"

#include <QtWidgets>
#include <QMultiMap>
#include <QVector>
#include <QByteArray>
#include <QMap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), frequencies(256, 0){

    QWidget *center = new QWidget();
    setCentralWidget(center);

    QVBoxLayout *mainLayout = new QVBoxLayout(center);

    QWidget *top = new QWidget();
    mainLayout->addWidget(top);

    QHBoxLayout *buttonBar = new QHBoxLayout(top);

    table = new QTableWidget();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(table);
    table->setColumnCount(4);
    table->setRowCount(256);
    for (int i = 0; i < 256; ++i){
        table->setRowHidden(i, true);
    }


    loadButton = new QPushButton("Load");
    buttonBar->addWidget(loadButton, 0, Qt::AlignCenter);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadButtonClicked);

    encodeButton = new QPushButton("encode");
    buttonBar->addWidget(encodeButton, 0, Qt::AlignCenter);
    connect(encodeButton, &QPushButton::clicked, this, &MainWindow::encodeButtonClicked);


    decodeButton = new QPushButton("decode");
    buttonBar->addWidget(decodeButton, 0, Qt::AlignCenter);
    connect(decodeButton, &QPushButton::clicked, this, &MainWindow::decodeButtonClicked);

}





MainWindow::~MainWindow() {}






void MainWindow::loadButtonClicked(){
    loadFileName = QFileDialog::getOpenFileName();
    QFile file(loadFileName);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::information(this, "Error", QString("Can't open file \"%1\"").arg(loadFileName));
        return;
    }

    else{
        fileSize = file.size();
        fileContents = file.readAll();

        if (fileContents.isEmpty()){
            QMessageBox::information(this, "Error", "File is Empty");
            return;
        }

        file.close();


        for (int iPos = 0; iPos < fileContents.length(); ++iPos){
            ++frequencies[(unsigned char) fileContents[iPos]];
        }


        for (int i = 0; i < 256; ++i){
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setData(Qt::DisplayRole, i);
            table->setItem(i, 0, item);
        }

        for (int i = 0; i < 256; ++i){
            QTableWidgetItem *item = new QTableWidgetItem(QChar(i));
            table->setItem(i, 1, item);
        }

        for (int i = 0; i < 256; ++i){
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setData(Qt::DisplayRole, frequencies[i]);
            table->setItem(i, 2, item);
            if (frequencies[i]){
                table->setRowHidden(i, false);
            }
        }

    }
}







void MainWindow::encodeButtonClicked(){

    QMultiMap<int, QByteArray> toDo;   //Maps a frequency to the QByteArray it corresponds to
    for (int code = 0; code < 256; ++code){
        if (frequencies[code] > 0){
            toDo.insert(frequencies[code], QByteArray(1, code));
        }
    }

    if (toDo.size() == 1){
        QMessageBox::information(this, "Error", QString("File can't be encoded because it only contains 1 character"));
        return;
    }

    QMap<QByteArray, QPair<QByteArray, QByteArray> > parentChildren;
    while (toDo.size() > 1) {
        int freq0 = toDo.begin().key();
        QByteArray chars0 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int freq1 = toDo.begin().key();
        QByteArray chars1 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int parentFreq = freq0 + freq1;
        QByteArray parentChars = chars0 + chars1;
        toDo.insert(parentFreq, parentChars);

        parentChildren[parentChars] = qMakePair(chars0, chars1);
    }


    QByteArray huffmanRoot = toDo.begin().value();



    QVector<QString> charCodeEncodingStrings(256, "");



    for (int i = 0; i < 256; ++i){
        if (frequencies[i]){
            QByteArray root = huffmanRoot;
            QString code = "";
            QByteArray target;
            target.append((unsigned char) i);

            if (root == target){
                charCodeEncodingStrings[i] = code;
            }
            else{
                while (root != target){
                    if(parentChildren[root].first.contains(target)){
                        code.append('0');
                        root = parentChildren[root].first;
                    }
                    else if (parentChildren[root].second.contains(target)){
                        code.append('1');
                        root = parentChildren[root].second;
                    }
                }
                charCodeEncodingStrings[i] = code;
            }
        }

        else charCodeEncodingStrings[i] = "";
    }


    //Add Items to table
    for (int i = 0; i < 256; ++i){
        QTableWidgetItem *item = new QTableWidgetItem(QString(charCodeEncodingStrings[i]));
        table->setItem(i, 3, item);
    }

    //Create Encoded String
    QString encodedString = "";
    for (int i = 0; i < fileContents.length(); ++i){
        int currentCharIndex = (unsigned char) fileContents[i];
        QString currentCode = charCodeEncodingStrings[currentCharIndex];
        encodedString.append(currentCode);
    }

    bitsToDecode = encodedString.length();


    //Convert byte String to Ints
    int pointer = 0;
    bool ok;
    QByteArray encodedInts;
    for (int i = 0; i < encodedString.length(); ++i){
        int currentByteInt;
        unsigned char currentByte;
        if (pointer > (encodedString.length() - 8)){
            currentByteInt = encodedString.mid(pointer, -1).toInt(&ok, 2);
            currentByte = static_cast<unsigned char>(currentByteInt);
            i = encodedString.length();
        }
        else{
            currentByteInt = encodedString.mid(pointer, 8).toInt(&ok, 2);
            currentByte = static_cast<unsigned char>(currentByteInt);
            pointer += 8;
        }
        encodedInts.append(currentByte);
    }



    QString outName = QFileDialog::getSaveFileName(this, "Save");

    QFile outFile(outName);

    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        QMessageBox::information(this, "Error", QString("Can't write to file \"%1\"").arg(outName));
        return;
    }


    QDataStream out (&outFile);

    out << charCodeEncodingStrings << bitsToDecode;

    out.writeRawData(encodedInts.data(), encodedInts.length());


    QMessageBox::information(this, "Info", QString("The original contains %1 bytes, and the new file contains %2 bytes").arg(fileSize).arg(outFile.size()));

    outFile.close();

}

void MainWindow::decodeButtonClicked(){

    decodeFileName = QFileDialog::getOpenFileName();
    QFile inFile(decodeFileName);

    if(!inFile.open(QIODevice::ReadOnly)){
        QMessageBox::information(this, "Error", QString("Can't open file \"%1\"").arg(decodeFileName));
        return;
    }

    else{

        QVector<QString> getKey;

        int decodeLength;

        QDataStream in (&inFile);

        in >> getKey >> decodeLength;


        QByteArray huffmanIn(((decodeLength + 7) / 8), '0');

        in.readRawData(huffmanIn.data(), decodeLength);



        QString toDecode = "";

        for (int i = 0, j = 0; i < ((decodeLength+7)/8), j < decodeLength; ++i, j+=8){
            if (j > decodeLength - 8){
                toDecode += QString::number((unsigned char)huffmanIn[i], 2).rightJustified((decodeLength - j), '0');
            }
            else{
                toDecode += QString::number((unsigned char)huffmanIn[i], 2).rightJustified(8, '0');
            }
        }



        QString decodedFileName = QFileDialog::getSaveFileName(this, "Save");

        QFile decodedFile(decodedFileName);

        if (!decodedFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            QMessageBox::information(this, "Error", QString("Can't write to file \"%1\"").arg(decodedFileName));
            return;
        }

        QDataStream decodeOut (&decodedFile);


        int i = 1;
        int decodePointer = 0;
        QString temp;

        while (i <= toDecode.length()){
            temp = toDecode.mid(decodePointer, i - decodePointer);
            if (getKey.contains(temp)){
                decodePointer += temp.length();
                i = decodePointer + 1;
                decodeOut << (char) getKey.indexOf(temp);
                temp = "";
            }
            else{
                ++i;
            }
        }


    }
}



































