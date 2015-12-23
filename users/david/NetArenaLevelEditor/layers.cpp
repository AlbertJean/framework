#include "layers.h"


#include "pallettes.h"
#include "grid.h"




LevelLayer::LevelLayer()
{
	m_x = m_y = 0;
}

MechLayer::MechLayer(BasePallette *bp) : LevelLayer()
{
	m_bp = bp;
}
MechLayer::~MechLayer()
{
}


void MechLayer::CreateLayer(int x, int y)
{
	m_pixmap = new QPixmap(x*BLOCKSIZE, y*BLOCKSIZE);
	m_pixmap->fill(Qt::transparent);

	m_grid = new unsigned int*[y];

	for(int i = 0; i < y; i++)
		m_grid[i] = new unsigned int[x];


	QPainter painter(m_pixmap);
	QPixmap* image;
	for(int y1 = 0; y1 < y; y1++)
		for(int x1 = 0; x1 < x; x1++)
		{
			m_grid[y1][x1] = ' ';

			ed.m_mecPallette;

			image = m_bp->GetImage(m_grid[y1][x1]);
			if(image)
				painter.drawPixmap(x*BLOCKSIZE,y*BLOCKSIZE ,*image);
		}

	m_x = x;
	m_y = y;
}

void MechLayer::SetElement(int x, int y, bool update)
{
	m_grid[y][x] = ed.GetCurrentPallette()->m_selectedID;


	QPainter painter(m_pixmap);
	QPixmap* image = m_bp->GetImage(m_grid[y][x]);
	if(image)
		painter.drawPixmap(x*BLOCKSIZE,y*BLOCKSIZE ,*image);

	if(update)
		ed.m_grid->update();
}

void MechLayer::DeleteElement(int x, int y, bool update)
{
	m_grid[y][x] = ' ';

	if(update)
		UpdateLayer();
}



#include <QImage>
void MechLayer::UpdateLayer()
{
	QPainter painter(m_pixmap);

	for(int y1 = 0; y1 < m_y; y1++)
		for(int x1 = 0; x1 < m_x; x1++)
		{
			QPixmap* image = m_bp->GetImage(m_grid[y1][x1]);
			if(image)
			{
				painter.drawPixmap(x1*BLOCKSIZE,y1*BLOCKSIZE ,*image);
			}
			else
				qDebug() << "Missing image from pallette with ID:" << (char)m_grid[y1][x1];
		}

	ed.m_grid->update(); //TODO move somewhere more appropriate
}

void MechLayer::SaveLayer(QString filename)
{
	QFile file(filename + ".txt");
	file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

	QTextStream out(&file);

	for(int y = 0; y < MAPY; y++)
	{
		for (int x = 0; x < MAPX; x++)
		{
			out << (char)(m_grid[y][x]);
		}
		if(y < (MAPY -1))
				out << endl;
	}
	file.close();
}

void MechLayer::LoadLayer(QString filename)
{
	QFile file(filename + ".txt");
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream in(&file);

	for(int y = 0; y < MAPY; y++)
	{
		for (int x = 0; x < MAPX; x++)
		{
			unsigned int key = in.read(1)[0].toLatin1();
			m_grid[y][x] = key;
		}
		if(y < (MAPY -1))
				in.read(1);
	}
	file.close();

	UpdateLayer();
}


ArtLayer::ArtLayer() : LevelLayer()
{
}
ArtLayer::~ArtLayer()
{
}

void ArtLayer::CreateLayer(int x, int y)
{
	m_pixmap = new QPixmap(x*BLOCKSIZE, y*BLOCKSIZE);
	m_pixmap->fill(Qt::transparent);
}

void ArtLayer::SetElement(int x, int y, bool update)
{
	if(update)
		UpdateLayer();
}

void ArtLayer::DeleteElement(int x, int y, bool update)
{
	if(update)
		UpdateLayer();
}

void ArtLayer::UpdateLayer()
{
}

bool testImageTransparancy(QImage image)
{
	image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

	for (int x = 0 ; x < image.width(); x++)
	{
		for (int y = 0 ; y < image.height(); y++)
			if (qAlpha(image.pixel(x, y)) != 0)
				return false;
	}

	return true;
}

void ArtLayer::SaveLayer(QString filename)
{
	QFile fileArt(filename + ".txt");

	fileArt.open(QIODevice::WriteOnly | QIODevice::Truncate);
	QDataStream out(&fileArt);
	out.setByteOrder(QDataStream::LittleEndian);


	out << VERSION;
	out << MAPX;
	out << MAPY;

	int count = 0;
	QImage temp;
	QList<QImage> templevel;
	QList<int> hitlist;
	for(int y = 0; y < MAPY; y++)
	{
		for (int x = 0; x < MAPX; x++)
		{
			temp = m_pixmap->copy(x*BLOCKSIZE, y*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE).toImage();
			if(!testImageTransparancy(temp))
			{
				hitlist.append(count);
				templevel.append(temp);
			}
			count++;
		}
	}

	out << hitlist.size();

	count = 0;
	int levelsize = MAPX*BLOCKSIZE;
	qDebug() << "levelsize calculated at " + levelsize;

	QImage artImage(levelsize, (((hitlist.size()*BLOCKSIZE)/levelsize)*BLOCKSIZE)+BLOCKSIZE, QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&artImage);

	while(!hitlist.empty())
	{
		int key = hitlist.front();
		painter.drawImage((count*BLOCKSIZE)%levelsize, ((count*BLOCKSIZE)/levelsize)*BLOCKSIZE, templevel.front());

		count++;
		out << key;
		hitlist.pop_front();
		templevel.pop_front();
	}

	artImage.save(filename + "data.png");
	fileArt.close();
}

void ArtLayer::LoadLayer(QString filename)
{
	QFile file(filename + ".txt");

	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);
	in.setByteOrder(QDataStream::LittleEndian);

	QPixmap image(filename + "data.png");

	int key = 0;
	int hitcount = 0;

	in >> key; //version
	in >> key; //mapx
	in >> key; //mapy
	in >> key; //count

	int levelsize = MAPX*BLOCKSIZE;
	qDebug() << "levelsize calculated at " + levelsize;

	while(!in.atEnd())
	{
		in >> key;
		QPixmap p(image.copy((hitcount*BLOCKSIZE)%levelsize, ((hitcount*BLOCKSIZE)/levelsize)*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE));

		QPainter painter(m_pixmap);
		painter.drawPixmap((key%MAPX)*BLOCKSIZE, (key/MAPX)*BLOCKSIZE, image.copy((hitcount*BLOCKSIZE)%levelsize, ((hitcount*BLOCKSIZE)/levelsize)*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE));

		hitcount++;
	}

	file.close();
}
