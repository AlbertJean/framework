#pragma once

#include <QString>
#include <QMap>

#include "QGraphicsPixmapItem"

class PreviewItem : public QGraphicsPixmapItem
{
public:
	PreviewItem();
	~PreviewItem();

	//virtual QRectF boundingRect() const;
};


class EditorView;
class QGraphicsView;
class QGraphicsScene;
class SettingsWidget;
class QGraphicsPixmapItem;
class BasePallette;
class LevelLayer;
class Grid;
class Template;
class TemplateScene;
class Ed
{
public:
	static Ed& I()
	{
		static Ed e;
		return e;
	}

	void Initialize();
	void LoadPallettes();


	EditorView* GetView();
	QGraphicsView* GetViewPallette();


    SettingsWidget* GetSettingsWidget();


	int GetMapX(){return m_mapx;}
	int GetMapY(){return m_mapy;}

	void SetMapXY(int x, int y);

	void LoadLevel(const QString& filename);
	void SaveLevel(const QString &filename);
	void SaveConfig(const QString& filename);
	void LoadConfig(const QString& filename);

    void NewLevel();

	void SetCurrentTemplate(Template* t);
	void ReturnToLevel();

	void UpdatePreview(int x, int y);

	Grid* m_grid;

	Template* m_level;
	Template* m_currentTarget;
	TemplateScene* m_templateScene;

	BasePallette* m_mecPallette;
	BasePallette* m_colPallette;

	PreviewItem* m_preview;



private:
	Ed(){}
	~Ed(){}

	SettingsWidget* m_settingsWidget;

	int m_mapx;
	int m_mapy;

	EditorView* m_view;
	QGraphicsView* m_viewPallette;

public:

	bool m_leftbuttonHeld;

	QString ArtFolderPath; //the root directory for all art and template textures


	QList<QString> GetLinesFromConfigFile(QString filename);
	QMap<QString, QString> SplitLines(const QList<QString>& lines);

	BasePallette* GetCurrentPallette();

	void SetCurrentPallette();
	void UpdateTransparancy();
};


