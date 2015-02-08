#pragma once


#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class Tile : public QGraphicsPixmapItem
{
public:
    Tile();
    virtual ~Tile();

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

    virtual void SetSelectedBlock(short block);

    short getBlock(){return selectedBlock;}

protected:
    short selectedBlock;
};


class TilePallette : public Tile
{
public:
    TilePallette ();
    virtual ~TilePallette ();

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e ){}
};

class TileProperties
{
public:
   TileProperties();
   ~TileProperties();


};

#include <QPair>
#include <QList>

class GameObject : public QGraphicsPixmapItem
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );

	void Load(QString data);
    void Load(QMap<QString, QString>& data);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

	void SetPos(QPointF pos);

    int x;
    int y;

	QColor q;

	int speed;

    int pivotx;
    int pivoty;


	QString type;
    QString texture;
	QString toText();

	QList<QPair<int, int> > path;

	bool palletteTile; //is this meant for ingame or used for pallette
};

class EditorScene : public QGraphicsScene
{
public:
    EditorScene();
    virtual ~EditorScene ();

    virtual void CreateLevel(int x, int y);
    virtual void InitializeLevel();
    void AddGrid();

    virtual void DeleteTiles();
    virtual void CreateTiles();

    virtual void CustomMouseEvent ( QGraphicsSceneMouseEvent * e );

    Tile** m_tiles;
	TileProperties** m_properties;

    int m_mapx;
    int m_mapy;
};

class EditorViewBasic : public QGraphicsView
{
public:
    EditorViewBasic ();
    virtual ~EditorViewBasic ();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *e);

    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

    int zoomLevel;

    void UpdateMatrix();


#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent * e);
#endif


};

class QSlider;
class EditorView : public EditorViewBasic
{
    Q_OBJECT
public:
    EditorView();
    virtual ~EditorView();


    QSlider* sliderOpacMech;
    QSlider* sliderOpacArt;
    QSlider* sliderOpacColl;
	QSlider* sliderOpacObject;

public slots:
    void Save();
    void Load();
    void New();
    void SwitchToMech(int s);
    void SwitchToArt(int s);
    void SwitchToCollission(int s);
	void SwitchToObject(int s);

    void SwitchToTemplateMode();

    void SetOpacityMech(int s);
    void SetOpacityArt(int s);
    void SetOpacityCollission(int s);
	void SetOpacityObject(int s);


    void SwitchToBigMap();

};

class EditorTemplate
{
public:

    struct TemplateTile
    {
        int x;
        int y;

        short blockMech;
        short blockArt;
        short blockColl;
    };

    QMap<QString, TemplateTile> m_list;

    QString m_name;

    void SaveTemplate();
};

class QTextEdit;
class QColorDialog;
class ObjectPropertyWindow : public QObject
{
	Q_OBJECT
public:
	ObjectPropertyWindow(){}
	virtual ~ObjectPropertyWindow(){}

	void CreateObjectPropertyWindow();

	void SetCurrentGameObject(GameObject* object);

    void UpdateObjectText();


	GameObject* GetCurrentGameObject();

	QWidget* m_w;
	QTextEdit* text;

	GameObject* currentObject;
    QColorDialog* picker;


public slots:
	void SaveToGameObject();
    void SetColor();

};


class QPushButton;
class NewMapWindow : public QWidget
{
    Q_OBJECT
public:
    NewMapWindow();
    ~NewMapWindow();

public slots:
    void NewMap();
    void CancelNewMap();


public:

    QPushButton* ok;
    QPushButton* cancel;

    QSlider* x;
    QSlider* y;

};
