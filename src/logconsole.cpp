#include "logconsole.h"
#include "logger.h"

#include <QModelIndex>
#include <QDebug>
#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

using namespace std;

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_LEFT 68
#define KEY_RIGHT 67
#define KEY_PLUS 43
#define KEY_MINUS 45

LogConsole::LogConsole(QObject *parent) : QObject(parent)
{
    _model = Logger::instance();
    for (int i = 0; i < _model->columnCount(); i++)
        headerWidths.insert(i, 10);

    connect(_model, &QAbstractItemModel::rowsInserted,
            [=](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent);
        Q_UNUSED(last);
        this->printRow(first);
    });

    struct termios old_tio, new_tio;
    unsigned char c;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    width = w.ws_col;
    height = w.ws_row;

    beginRow = 0;
    lines = height - _model->columnCount() - 3;
    currentColumn = -1;

    CinReader *reader = new CinReader;

    connect(reader, &CinReader::upPressed, [=]() {
        if (beginRow > 0) {
            beginRow--;
            this->printScreen();
        }
    });
    connect(reader, &CinReader::downPressed, [=]() {
        if (beginRow < _model->rowCount() - 1) {
            beginRow++;
            this->printScreen();
        }
    });

    connect(reader, &CinReader::leftPressed, [=]() {
        if (currentColumn > 0) {
            currentColumn--;
            this->printScreen();
        }
    });
    connect(reader, &CinReader::rightPressed, [=]() {
        if (currentColumn < _model->columnCount() - 1) {
            currentColumn++;
            this->printScreen();
        }
    });
    connect(reader, &CinReader::plusPressed, [=]() {
        if (currentColumn >= 0 && currentColumn < _model->columnCount() - 1) {
            headerWidths[currentColumn]++;
            this->printScreen();
        }
    });
    connect(reader, &CinReader::minusPressed, [=]() {
        if (currentColumn >= 0 && currentColumn < _model->columnCount() - 1) {
            headerWidths[currentColumn]--;
            this->printScreen();
        }
    });

    printScreen();
    reader->start();
}

void LogConsole::printScreen()
{
    system("clear");
    for (int i = 0; i < headerWidths.size(); i++)
        cout << std::left << setw(headerWidths[i])
             << _model->headerData(i, Qt::Horizontal)
                    .toString()
                    .toLatin1()
                    .data();
    cout << endl;
    for (int i = beginRow; i < beginRow + lines; i++)
        printRow(i);

    for (int i = 0; i < width; i++)
        cout << "―";
    cout << endl;

    printSummry();
}

void LogConsole::printRow(int row)
{
    for (int i = 0; i < headerWidths.size(); i++) {
        QString s = _model->data(_model->index(row, i, QModelIndex()),
                                 Qt::DisplayRole).toString();
        cutText(s, headerWidths.value(i, 10));

        if (currentColumn == i)
            cout << "\033[1;31m";
        cout << std::left << setw(headerWidths[i]) << s.toLatin1().data();
        if (currentColumn == i)
            cout << "\033[0m";
    }
    cout << endl;
}

void LogConsole::printSummry()
{
    for (int i = 0; i < _model->columnCount(); i++) {
        cout << setw(10)
             << _model->headerData(i, Qt::Horizontal)
                    .toString()
                    .toLatin1()
                    .data()
             << _model->data(_model->index(beginRow, i, QModelIndex()),
                             Qt::DisplayRole)
                    .toString()
                    .toLatin1()
                    .data() << endl;
    }
}

void LogConsole::cutText(QString &s, int len)
{
    if (s.length() > len)
        s = s.left(len);
}

void CinReader::run()
{
    forever
    {
        int i = getchar();
        switch (i) {

        case KEY_UP:
            emit upPressed();
            break;
        case KEY_DOWN:
            emit downPressed();
            break;
        case KEY_RIGHT:
            emit rightPressed();
            break;
        case KEY_LEFT:
            emit leftPressed();
            break;
        case KEY_PLUS:
            emit plusPressed();
            cin.clear();
            break;
        case KEY_MINUS:
            emit minusPressed();
            cin.clear();
            break;

//        default:
//            cout<<"*"<<i<<"*"<<endl;
        }
    }
}
