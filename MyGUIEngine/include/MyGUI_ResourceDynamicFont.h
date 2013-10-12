/*
	���ļ���Ҫ�������ĵȶ��ַ����ԣ�������ResourceTrueTypeFontһ����
	����ȫ�����ַ�ͼ�񣬶���һ�ֻ��ģʽ������ʼ������һ���Զ���һ����
	�ַ�ͼ��ͬʱ���û��ڵ���getGlyphInfoʱû�з����ַ�����ʱ���ཫ
	��ʱ��������ַ�ͼ��
	��������Ǵ�ResourceTrueTypeFont�޸Ķ��������ұ�����������֯��ʽ
	�޸��ļ�MyGUI_FontManager.cpp�ں���void FontManager::initialise()��

	a.�������е���
	FactoryManager::getInstance().registerFactory<ResourceDynamicFont>(XML_TYPE_RESOURCE);

	b.��OgreTexture�м��뺯��(MyGUI_OgreTexture.cpp)

	//begin
	void* OgreTexture::lock(int l,int t,int r,int b,TextureUsage _access)
	{
		if (_access == TextureUsage::Write)
		{
			return mTexture->getBuffer()->lock(Ogre::Image::Box(l,t,r,b),Ogre::HardwareBuffer::HBL_DISCARD).data;
		}
		//�������ڶ�̬�����д�����
		return nullptr;
	}
	
	bool OgreTexture::resize(int width,int height)
	{
		//ɾ���ϵĲ���
		Ogre::TextureManager::getSingleton().remove(mTexture->getName());

		//�����µĲ���
		Ogre::TexturePtr newTex = Ogre::TextureManager::getSingleton().createManual(
			mName,
			mGroup,
			Ogre::TEX_TYPE_2D,
			width, 
			height,
			0,
			mPixelFormat,
			mUsage,
			this);
		newTex->load();
		//���ϵĲ���blit���µĲ�����
		Ogre::Image::Box box(0,0,std::min(width,getWidth()),std::min(height,getHeight()));
		newTex->getBuffer()->blit( mTexture->getBuffer(),box,box );
		
		mTexture = newTex;
		return true;
	}
	//end

	��IFont�ӿ��м���
	virtual void* lock(int l,int t,int r,int b,TextureUsage _access) = 0;
	virtual bool resize(int width,int height) = 0;

	d.Ϊ��ʵ��һ�ֶ�̬�������������ģʽ����Ҫ���������ļ�
		EditTextManager.h
		EditTextManager.cpp
		�����޸Ĳο�EditTextManager.h��ͷ����
*/

#ifndef __MYGUI_RESOURCE_DYNAMIC_FONT_H__
#define __MYGUI_RESOURCE_DYNAMIC_FONT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ITexture.h"
#include "MyGUI_IFont.h"

#ifdef MYGUI_USE_FREETYPE
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#include FT_GLYPH_H
	#include FT_BITMAP_H
#endif // MYGUI_USE_FREETYPE

namespace MyGUI
{

	class MYGUI_EXPORT ResourceDynamicFont :
		public IFont
	{
		MYGUI_RTTI_DERIVED( ResourceDynamicFont )

	public:
		typedef std::vector<PairCodePoint> VectorPairCodePoint;
		typedef std::vector<RangeInfo> VectorRangeInfo;
		typedef std::vector<PairCodeCoord> VectorPairCodeCoord;

		ResourceDynamicFont();
		virtual ~ResourceDynamicFont();

		virtual void deserialization(xml::ElementPtr _node, Version _version);

		virtual GlyphInfo* getGlyphInfo(Char _id);

		virtual ITexture* getTextureFont();

		// ���ݧ��ڧӧ�ѧ��� �ӧ����� ���� �ԧ֧ߧ֧�ѧ�ڧ� �� ��ڧܧ�֧ݧ��
		virtual int getDefaultHeight();

		void setStyle( IFont::Style _style );
	private:
		void addCodePointRange(Char _first, Char _second);
		void addHideCodePointRange(Char _first, Char _second);

		void setItalic();
		void setBold();

		// ����ӧ֧��֧�, �ӧ��էڧ� �ݧ� ��ڧާӧ�� �� �٧�ߧ� �ߧ֧ߧ�اߧ�� ��ڧާӧ�ݧ��
		bool checkHidePointCode(Char _id);

		/** Clear the list of code point ranges. */
		void clearCodePointRanges();

		void initialise();

		void addGlyph(GlyphInfo* _info, Char _index,
			int _left, int _top, int _right, int _bottom, 
			int _finalw, int _finalh, float _aspect, int _addHeight = 0,
			float _advance = 0.0f,float _bearingX = 0.0f );

		uint8* writeData(uint8* _pDest, unsigned char _luminance, unsigned char _alpha, bool _rgba);

		//����һ���µ�����
		GlyphInfo* addGlyphInfo( Char _id );
		//����һ���µ�
		bool newGlyphInfo( GlyphInfo* pgi,Char _id );
		//���¶�λ�������������
		void relocal( float sx,float sy );
		void reLocalGlyph( GlyphInfo& gi,float sx,float sy);
	private:
		// Source of the font
		std::string mSource;
		// Size of the truetype font, in points
		float mTtfSize;
		// Resolution (dpi) of truetype font
		uint mTtfResolution;

		bool mAntialiasColour;

		int mDistance;
		int mSpaceWidth;
		int mTabWidth;
		int mCursorWidth;
		int mSelectionWidth;
		int mOffsetHeight;
		int mHeightPix;

		// ���է֧ݧ�ߧѧ� �ڧߧ���ާѧ�ڧ� �� ��ڧާӧ�ݧѧ�
		GlyphInfo mSpaceGlyphInfo;
		GlyphInfo mTabGlyphInfo;
		GlyphInfo mSelectGlyphInfo;
		GlyphInfo mSelectDeactiveGlyphInfo;
		GlyphInfo mCursorGlyphInfo;

		// ��ڧާӧ�ݧ� �ܧ������ �ߧ� �ߧ�اߧ� ��ڧ��ӧѧ��
		VectorPairCodePoint mVectorHideCodePoint;

		// �ӧ�� �ڧߧ���ާѧ�ڧ� �� ��ڧާӧ�ݧѧ�
		/*
			�ֱ����4��ģʽNormal,Bold,Italic,BoldItalic
		*/
		VectorRangeInfo mVectorRangeInfo[4];

		MyGUI::ITexture* mTexture;

		//���建��
		FT_Library mftLibrary;
		FT_Face face;
		/*��initialise�е����ݱ�����������newGlyphInfo��ʹ��
		*/
		struct Context
		{
			int length;
			int height; //���Ƶ�λ��
			int max_height;

			int finalHeight;
			int finalWidth;

			int max_bear;

			size_t pixel_bytes;
			FT_Int advance;

			float textureAspect;

			bool rgbaMode;
		};
		Context mContext;
		bool mIsFull; //�ռ�����
		//�����ӵ�����
		int mWidth;
		int mHeight;

		IFont::Style mCurrentStyle;
	};

} // namespace MyGUI

#endif // __MYGUI_RESOURCE_TRUE_TYPE_FONT_H__
