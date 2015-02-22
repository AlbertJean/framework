#include <cmath>
#include <map>
#include <string>
#include <tinyxml2.h>
#include <vector>
#include "Debugging.h"
#include "framework.h"
#include "spriter.h"

// todo : create a nice drawing function
// todo : remove all of the dynamic memory allocations during draw
// todo : more asserts
// todo : add support for character map

using namespace tinyxml2;

namespace spriter
{
	class Transform;

	// helper functions

	static float toRadians(float degrees);
	static float linear(float a, float b, float t);
	static Transform linear(const Transform & infoA, const Transform & infoB, int spin, float t);
	static float angleLinear(float angleA, float angleB, int spin, float t);
	static float quadratic(float a, float b, float c, float t);
	static float cubic(float a, float b, float c, float d, float t);

	//

	enum LoopType
	{
		kLoopType_Looping,
		kLoopType_NoLooping
	};

	enum ObjectType
	{
		kObjectType_Sprite,
		kObjectType_Bone,
		kObjectType_Box,
		kObjectType_Point,
		kObjectType_Sound,
		kObjectType_Entity,
		kObjectType_Variable
	};

	enum CurveType
	{
		kCurveType_Instant,
		kCurveType_Linear,
		kCurveType_Quadratic,
		kCurveType_Cubic
	};

	//

	class Transform
	{
	public:
		float x;
		float y;
		float angle;
		float scaleX;
		float scaleY;
		float a;

		Transform()
			: x(0.f)
			, y(0.f)
			, angle(0.f)
			, scaleX(1.f)
			, scaleY(1.f)
			, a(1.f)
		{
		}

		void multiply(const Transform & parentInfo, Transform & result) const
		{
			if (x != 0.f || y != 0.f)
			{
				const float preMultX = x * parentInfo.scaleX;
				const float preMultY = y * parentInfo.scaleY;

				const float s = std::sin(toRadians(parentInfo.angle));
				const float c = std::cos(toRadians(parentInfo.angle));

				result.x = (preMultX * c) - (preMultY * s);
				result.y = (preMultX * s) + (preMultY * c);
				result.x += parentInfo.x;
				result.y += parentInfo.y;
			}
			else
			{
				result.x = parentInfo.x;
				result.y = parentInfo.y;
			}

			result.angle = angle + parentInfo.angle;
			result.scaleX = scaleX * parentInfo.scaleX;
			result.scaleY = scaleY * parentInfo.scaleY;
			result.a = a * parentInfo.a;
		}
	};

	class File
	{
	public:
		std::string name;
		int width;
		int height;
		float pivotX;
		float pivotY;

		File()
			: width(0)
			, height(0)
			, pivotX(0.f)
			, pivotY(1.f)
		{
		}
	};

	typedef std::pair<int, int> FolderAndFile;

	class FileCache
	{
	public:
		std::map<FolderAndFile, File> files;
	};

#if 0
	class MapInstruction
	{
	public:
		int folder;
		int file;
		int targetFolder;
		int targetFile;

		MapInstruction()
			: folder(0)
			, file(0)
			, targetFolder(-1)
			, targetFile(-1)
		{
		}
	};

	class CharacterMap
	{
	public:
		std::string name;
		std::vector<MapInstruction> maps;
	};
#endif

	class MainlineKey
	{
	public:
		class Ref
		{
		public:
			int parent;
			int timeline;
			int key;

			Ref()
				: parent(-1)
				, timeline(0)
				, key(0)
			{
			}
		};

		int time;
		std::vector<Ref> boneRefs;
		std::vector<Ref> objectRefs;

		MainlineKey()
			: time(0)
		{
		}
	};

	class TimelineKey
	{
	public:
		int time;
		int spin;
		CurveType curveType;
		float c1;
		float c2;

		mutable int refCount;

		TimelineKey()
			: time(0)
			, spin(1)
			, curveType(kCurveType_Linear)
			, c1(0.f)
			, c2(0.f)
			, refCount(1)
		{
		}

		virtual ~TimelineKey()
		{
		}

		void AddRef() const
		{
			refCount++;
		}

		void Release() const
		{
			refCount--;

			if (refCount == 0)
			{
				delete this;
			}
		}

		TimelineKey * interpolate(const TimelineKey * nextKey, int nextKeyTime, float currentTime) const
		{
			return linear(nextKey, getTWithNextKey(nextKey, nextKeyTime, currentTime));
		}

		float getTWithNextKey(const TimelineKey * nextKey, int nextKeyTime, float currentTime) const
		{
			if (curveType == kCurveType_Instant || time == nextKeyTime)
			{
				return 0.f;
			}

			const float t = (currentTime - time) / (nextKeyTime - time);
			Assert(t >= 0.f && t <= 1.f);

			if (curveType == kCurveType_Linear)
			{
				return t;
			}
			else if (curveType == kCurveType_Quadratic)
			{
				return quadratic(0.f, c1, 1.f, t);
			}
			else if (curveType == kCurveType_Cubic)
			{
				return cubic(0.f, c1, c2, 1.f, t);
			}

			Assert(false);
			return 0.f;
		}

		virtual TimelineKey * linear(const TimelineKey * keyB, float t) const = 0;
	};

	class Timeline
	{
	public:
		std::string name;
		ObjectType objectType;
		std::vector<TimelineKey*> keys;

		Timeline()
			: objectType(kObjectType_Sprite)
		{
		}

		~Timeline()
		{
			// fixme : cannot copy Timeline during load..
			//for (size_t i = 0; i < keys.size(); ++i)
			//	delete keys[i];
			//keys.clear();
		}
	};

	class SpatialTimelineKey : public TimelineKey
	{
	public:
		Transform transform;

		virtual ~SpatialTimelineKey()
		{
		}
	};

	class BoneTimelineKey : public SpatialTimelineKey
	{
	public:
		int length;
		int width;

		BoneTimelineKey()
			: SpatialTimelineKey()
			, length(200)
			, width(10)
		{
		}

		virtual TimelineKey * linear(const TimelineKey * _keyB, float t) const
		{
			const BoneTimelineKey * keyB = dynamic_cast<const BoneTimelineKey*>(_keyB);
			Assert(keyB);

			BoneTimelineKey * result = new BoneTimelineKey();
			*result = *this;
			result->transform = spriter::linear(transform, keyB->transform, spin, t);

			return result;
		}
	};

	class SpriteTimelineKey : public SpatialTimelineKey
	{
	public:
		int folder;
		int file;
		bool useDefaultPivot; // true if missing pivot_x and pivot_y in object tag
		float pivotX;
		float pivotY;

		SpriteTimelineKey()
			: SpatialTimelineKey()
			, folder(0)
			, file(0)
			, useDefaultPivot(false)
			, pivotX(0.f)
			, pivotY(1.f)
		{
		}

		virtual TimelineKey * linear(const TimelineKey * _keyB, float t) const
		{
			const SpriteTimelineKey * keyB = dynamic_cast<const SpriteTimelineKey*>(_keyB);
			Assert(keyB);

			SpriteTimelineKey * result = new SpriteTimelineKey();
			*result = *this;
			result->transform = spriter::linear(transform, keyB->transform, spin, t);

			if (!useDefaultPivot)
			{
				result->pivotX = spriter::linear(pivotX, keyB->pivotX, t);
				result->pivotY = spriter::linear(pivotY, keyB->pivotY, t);
			}

			return result;
		}
	};

	struct TransformedObjectKey
	{
		const SpriteTimelineKey * key;
		Transform transform;
	};

	class Animation
	{
	public:
		std::string name;
		int length;
		LoopType loopType;
		std::vector<MainlineKey> mainlineKeys;
		std::vector<Timeline> timelines;

		Animation()
			: length(0)
			, loopType(kLoopType_Looping)
		{
		}

		std::vector<TransformedObjectKey> getAnimationDataAtTime(float newTime)
		{
			if (loopType == kLoopType_NoLooping)
			{
				newTime = std::min<float>(newTime, length);
			}
			else if (loopType == kLoopType_Looping)
			{
				newTime = std::fmodf(newTime, length);
			}

			const MainlineKey & mainKey = mainlineKeyFromTime(newTime);

			struct TransformedBoneKey
			{
				const BoneTimelineKey * key;
				Transform transform;
			};

			std::vector<TransformedBoneKey> transformedBoneKeys;

			for (size_t b = 0; b < mainKey.boneRefs.size(); ++b)
			{
				Transform parentTransform;

				const auto & currentRef = mainKey.boneRefs[b];

				if (currentRef.parent >= 0)
				{
					parentTransform = transformedBoneKeys[currentRef.parent].transform;
				}
				else
				{
					parentTransform = Transform(); // todo : set to root node properties of game object
				}

				TransformedBoneKey transformedKey;
				transformedKey.key = dynamic_cast<const BoneTimelineKey*>(keyFromRef(currentRef, newTime));
				transformedKey.key->transform.multiply(parentTransform, transformedKey.transform);
				transformedBoneKeys.push_back(transformedKey);
			}

			//

			std::vector<TransformedObjectKey> objectKeys;

			for (size_t o = 0; o < mainKey.objectRefs.size(); ++o)
			{
				Transform parentTransform;

				const auto & currentRef = mainKey.objectRefs[o];

				if (currentRef.parent >= 0)
				{
					parentTransform = transformedBoneKeys[currentRef.parent].transform;
				}
				else
				{
					parentTransform = Transform(); // todo : set to root node properties of game object
				}

				TransformedObjectKey transformedKey;
				transformedKey.key = dynamic_cast<const SpriteTimelineKey*>(keyFromRef(currentRef, newTime));
				transformedKey.key->transform.multiply(parentTransform, transformedKey.transform);
				objectKeys.push_back(transformedKey);
			}

			for (auto k : transformedBoneKeys)
				k.key->Release();

			return objectKeys;
		}

		const MainlineKey & mainlineKeyFromTime(int currentTime) const
		{
			int currentMainKey = 0;

			for (size_t m = 0; m < mainlineKeys.size(); ++m)
			{
				if (mainlineKeys[m].time <= currentTime)
				{
					currentMainKey = m;
				}

				if (mainlineKeys[m].time >= currentTime)
				{
					break;
				}
			}

			return mainlineKeys[currentMainKey];
		}

		const TimelineKey * keyFromRef(const MainlineKey::Ref & ref, float newTime) const
		{
			const Timeline & timeline = timelines[ref.timeline];
			const TimelineKey * keyA = timeline.keys[ref.key];

			if (timeline.keys.size() == 1)
			{
				keyA->AddRef();
				return keyA;
			}

			size_t nextKeyIndex = ref.key + 1;

			if (nextKeyIndex >= timeline.keys.size())
			{
				if (loopType == kLoopType_Looping)
				{
					nextKeyIndex = 0;
				}
				else
				{
					keyA->AddRef();
					return keyA;
				}
			}

			const TimelineKey * keyB = timeline.keys[nextKeyIndex];
			int keyBTime = keyB->time;

			if (keyBTime < keyA->time)
			{
				keyBTime = keyBTime + length;
			}

			return keyA->interpolate(keyB, keyBTime, newTime);
		}
	};

	// helper functions

	static float toRadians(float degrees)
	{
		return degrees / 180.f * M_PI;
	}

	static float linear(float a, float b, float t)
	{
		return ((b - a) * t) + a;
	}

	static Transform linear(const Transform & infoA, const Transform & infoB, int spin, float t)
	{
		Transform result;
		result.x = linear(infoA.x, infoB.x, t);
		result.y = linear(infoA.y, infoB.y, t);
		result.angle = angleLinear(infoA.angle, infoB.angle, spin, t);
		result.scaleX = linear(infoA.scaleX, infoB.scaleX, t);
		result.scaleY = linear(infoA.scaleY, infoB.scaleY, t);
		result.a = linear(infoA.a, infoB.a, t);
		return result;
	}

	static float angleLinear(float angleA, float angleB, int spin, float t)
	{
		if (spin == 0)
		{
			return angleA;
		}

		if (spin > 0)
		{
			if ((angleB - angleA) < 0)
			{
				angleB += 360;
			}
		}
		else if (spin < 0)
		{
			if ((angleB - angleA) > 0)
			{
				angleB -= 360;
			}
		}

		return linear(angleA, angleB, t);
	}

	static float quadratic(float a, float b, float c, float t)
	{
		return linear(linear(a, b, t), linear(b, c, t), t);
	}

	static float cubic(float a, float b, float c, float d, float t)
	{
		return linear(quadratic(a, b, c, t), quadratic(b, c, d, t), t);
	}
	
	// tinyxml helper functions

	static const char * stringAttrib(XMLElement * elem, const char * name, const char * defaultValue)
	{
		if (elem->Attribute(name))
			return elem->Attribute(name);
		else
			return defaultValue;
	}

	static int boolAttrib(XMLElement * elem, const char * name, bool defaultValue)
	{
		if (elem->Attribute(name))
			return elem->BoolAttribute(name);
		else
			return defaultValue;
	}

	static int intAttrib(XMLElement * elem, const char * name, int defaultValue)
	{
		if (elem->Attribute(name))
			return elem->IntAttribute(name);
		else
			return defaultValue;
	}

	static float floatAttrib(XMLElement * elem, const char * name, float defaultValue)
	{
		if (elem->Attribute(name))
			return elem->FloatAttribute(name);
		else
			return defaultValue;
	}

	static void readTimelineKeyProperties(XMLElement * xmlKey, TimelineKey * key)
	{
		// id, time, spin, curve_type, c1, c2

		key->time = xmlKey->IntAttribute("time");
		key->spin = intAttrib(xmlKey, "spin", 1);
		key->curveType = (CurveType)intAttrib(xmlKey, "curve_type", kCurveType_Linear);
		key->c1 = xmlKey->FloatAttribute("c1");
		key->c2 = xmlKey->FloatAttribute("c2");
	}

	//

	Entity::Entity()
		: m_scene(0)
	{
	}

	Entity::~Entity()
	{
		for (size_t i = 0; i < m_animations.size(); ++i)
			delete m_animations[i];
		m_animations.clear();
	}

	int Entity::getAnimIndexByName(const char * name) const
	{
		for (size_t i = 0; i < m_animations.size(); ++i)
			if (m_animations[i]->name == name)
				return i;
		return -1;
	}

	int Entity::getAnimLength(int index) const
	{
		return m_animations[index]->length;
	}

	void Entity::getDrawableListAtTime(int animIndex, float time, SpriterDrawable * drawables, int & numDrawables) const
	{
		Animation * animation = m_animations[animIndex];

		std::vector<TransformedObjectKey> keys = animation->getAnimationDataAtTime(time);

		numDrawables = std::min(numDrawables, (int)keys.size());

		for (size_t k = 0; k < numDrawables; ++k)
		{
			const TransformedObjectKey & o = keys[k];

			const Transform & tf = o.transform;

			FolderAndFile ff;
			ff.first = o.key->folder;
			ff.second = o.key->file;
			const File & file = m_scene->m_fileCache->files[ff];

			SpriterDrawable & drawable = drawables[k];

			drawable.filename = file.name.c_str();
			drawable.x = tf.x;
			drawable.y = -tf.y;
			drawable.angle = -tf.angle;
			drawable.scaleX = tf.scaleX;
			drawable.scaleY = tf.scaleY;
			drawable.pivotX = (o.key->useDefaultPivot ? file.pivotX : o.key->pivotX) * file.width;
			drawable.pivotY = (1.f - (o.key->useDefaultPivot ? file.pivotY : o.key->pivotY)) * file.height;
			drawable.a = tf.a;

			o.key->Release();
		}
	}

	//

	Scene::Scene()
		: m_fileCache(0)
	{
	}

	Scene::~Scene()
	{
		for (size_t i = 0; i < m_entities.size(); ++i)
			delete m_entities[i];
		m_entities.clear();

		delete m_fileCache;
		m_fileCache = 0;
	}

	bool Scene::load(const char * filename)
	{
		bool result = true;

		filename = "Testcharacter.scml";
		//filename = "spriter-test.scml";
		changeDirectory("C:\\Users\\Marcel\\Google Drive\\NetArena\\ArtistCave\\JoyceTestcharacter\\TestCharacter_Spriter");

		// todo : fill file cache with files relative to SCML file

		XMLDocument xmlModelDoc;
		
		if (xmlModelDoc.LoadFile(filename) != XML_NO_ERROR)
		{
			logError("failed to load %s", filename);

			result = false;
		}
		else
		{
			XMLElement * xmlSpriterData = xmlModelDoc.FirstChildElement("spriter_data");

			if (xmlSpriterData == 0)
			{
				logError("missing <spriter_data> element");

				result = false;
			}
			else
			{
				m_fileCache = new FileCache();

				int folderIndex = 0;

				for (XMLElement * xmlFolder = xmlSpriterData->FirstChildElement("folder"); xmlFolder; xmlFolder = xmlFolder->NextSiblingElement("folder"))
				{
					// id, name

					int fileIndex = 0;

					for (XMLElement * xmlFile = xmlFolder->FirstChildElement("file"); xmlFile; xmlFile = xmlFile->NextSiblingElement("file"))
					{
						// id, name, width, height, pivot_x, pivot_y

						File file;

						file.name = stringAttrib(xmlFile, "name", "");
						file.width = intAttrib(xmlFile, "width", 0);
						file.height = intAttrib(xmlFile, "height", 0);
						file.pivotX = intAttrib(xmlFile, "pivot_x", 0);
						file.pivotY = intAttrib(xmlFile, "pivot_y", 0);

						FolderAndFile ff;
						ff.first = folderIndex;
						ff.second = fileIndex;

						m_fileCache->files[ff] = file;

						fileIndex++;
					}

					folderIndex++;
				}

				for (XMLElement * xmlEntity = xmlSpriterData->FirstChildElement("entity"); xmlEntity; xmlEntity = xmlEntity->NextSiblingElement("entity"))
				{
					// id, name

					Entity * entity = new Entity();

					entity->m_name = stringAttrib(xmlEntity, "name", "");

					// <character_map>
					// id, name
					//     <map>
					//     folder, file, target_folder, target_file

					for (XMLElement * xmlAnimation = xmlEntity->FirstChildElement("animation"); xmlAnimation; xmlAnimation = xmlAnimation->NextSiblingElement("animation"))
					{
						// id, name, length, looping

						Animation * animation = new Animation();

						animation->name = stringAttrib(xmlAnimation, "name", "");
						animation->length = xmlAnimation->IntAttribute("length");
						animation->loopType = boolAttrib(xmlAnimation, "looping", true) ? kLoopType_Looping : kLoopType_NoLooping;

						XMLElement * xmlMainline = xmlAnimation->FirstChildElement("mainline");

						if (xmlMainline)
						{
							for (XMLElement * xmlKey = xmlMainline->FirstChildElement("key"); xmlKey; xmlKey = xmlKey->NextSiblingElement("key"))
							{
								// id, time

								MainlineKey key;

								key.time = xmlKey->IntAttribute("time");

								for (XMLElement * xmlObjectRef = xmlKey->FirstChildElement("object_ref"); xmlObjectRef; xmlObjectRef = xmlObjectRef->NextSiblingElement("object_ref"))
								{
									// id, parent, timeline, key

									MainlineKey::Ref ref;
									if (xmlObjectRef->QueryIntAttribute("parent", &ref.parent) != XML_NO_ERROR)
										ref.parent = -1;
									ref.timeline = xmlObjectRef->IntAttribute("timeline");
									ref.key = xmlObjectRef->IntAttribute("key");

									key.objectRefs.push_back(ref);
								}

								for (XMLElement * xmlBoneRef = xmlKey->FirstChildElement("bone_ref"); xmlBoneRef; xmlBoneRef = xmlBoneRef->NextSiblingElement("bone_ref"))
								{
									// id, parent, timeline, key

									MainlineKey::Ref ref;
									if (xmlBoneRef->QueryIntAttribute("parent", &ref.parent) != XML_NO_ERROR)
										ref.parent = -1;
									ref.timeline = xmlBoneRef->IntAttribute("timeline");
									ref.key = xmlBoneRef->IntAttribute("key");

									key.boneRefs.push_back(ref);
								}

								animation->mainlineKeys.push_back(key);
							}
						}

						for (XMLElement * xmlTimeline = xmlAnimation->FirstChildElement("timeline"); xmlTimeline; xmlTimeline = xmlTimeline->NextSiblingElement("timeline"))
						{
							// id, name, type

							Timeline timeline;

							timeline.name = stringAttrib(xmlTimeline, "name", "");

							std::string type = stringAttrib(xmlTimeline, "type", "");
							if (type == "sprite")
								timeline.objectType = kObjectType_Sprite;

							for (XMLElement * xmlKey = xmlTimeline->FirstChildElement(); xmlKey; xmlKey = xmlKey->NextSiblingElement())
							{
								XMLElement * xmlKeyData = xmlKey->FirstChildElement();

								if (xmlKeyData)
								{
									const char * keyName = xmlKeyData->Name();

									if (!strcmp(keyName, "object"))
									{
										SpriteTimelineKey * key = new SpriteTimelineKey();

										readTimelineKeyProperties(xmlKey, key);

										// folder, file, x, y, angle, scale_x, scale_y, pivot_x, pivot_y

										key->folder = xmlKeyData->IntAttribute("folder");
										key->file = xmlKeyData->IntAttribute("file");
										key->transform.x = xmlKeyData->FloatAttribute("x");
										key->transform.y = xmlKeyData->FloatAttribute("y");
										key->transform.angle = xmlKeyData->FloatAttribute("angle");
										key->transform.scaleX = floatAttrib(xmlKeyData, "scale_x", 1.f);
										key->transform.scaleY = floatAttrib(xmlKeyData, "scale_y", 1.f);
										key->transform.a = floatAttrib(xmlKeyData, "a", 1.f);

										if (xmlKeyData->Attribute("pivot_x") || xmlKeyData->Attribute("pivot_y"))
										{
											key->useDefaultPivot = false;

											key->pivotX = xmlKeyData->FloatAttribute("pivot_x");
											key->pivotY = xmlKeyData->FloatAttribute("pivot_y");
										}
										else
										{
											key->useDefaultPivot = true;
										}

										timeline.keys.push_back(key);
									}
									else if (!strcmp(keyName, "bone"))
									{
										BoneTimelineKey * key = new BoneTimelineKey();

										readTimelineKeyProperties(xmlKey, key);

										// x, y, angle, scale_x, scale_y, a

										key->transform.x = xmlKeyData->FloatAttribute("x");
										key->transform.y = xmlKeyData->FloatAttribute("y");
										key->transform.angle = xmlKeyData->FloatAttribute("angle");
										key->transform.scaleX = floatAttrib(xmlKeyData, "scale_x", 1.f);
										key->transform.scaleY = floatAttrib(xmlKeyData, "scale_y", 1.f);
										key->transform.a = floatAttrib(xmlKeyData, "a", 1.f);

										timeline.keys.push_back(key);
									}
								}
							}

							animation->timelines.push_back(timeline);
						}

						entity->m_animations.push_back(animation);
					}

					m_entities.push_back(entity);
				}
			}
		}

		return result;
	}

	int Scene::getEntityIndexByName(const char * name) const
	{
		for (size_t i = 0; i < m_entities.size(); ++i)
			if (m_entities[i]->m_name == name)
				return i;
		return -1;
	}
}
