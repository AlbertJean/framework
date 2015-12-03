#pragma once

#include <QString>


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


    EditorView*& GetView();
    QGraphicsView*& GetViewPallette();


    SettingsWidget* GetSettingsWidget();


	int GetMapX(){return m_mapx;}
	int GetMapY(){return m_mapy;}

	void LoadLevel(const QString& filename);
	void SaveLevel(const QString &filename);

    void NewLevel();

	Grid* m_grid;

	Template* m_level;
	Template* m_currentTemplate;
    Template* m_levelbackup;
    TemplateScene* m_templateScene;



private:
	Ed(){}
	~Ed(){}

    SettingsWidget* m_settingsWidget;

	int m_mapx;
	int m_mapy;

	EditorView* m_view;
	QGraphicsView* m_viewPallette;

	BasePallette* m_mecPallette;
	BasePallette* m_colPallette;
    //TemplatePallette* m_templatePallette;

public:

	bool m_leftbuttonHeld;

	QString ObjectPath; //the directory for all object textures
	QString ArtFolderPath; //the root directory for all art and template textures


	QList<QString> GetLinesFromConfigFile(QString filename);

	BasePallette* GetCurrentPallette();

	void SetCurrentPallette();
	void UpdateTransparancy();
};


