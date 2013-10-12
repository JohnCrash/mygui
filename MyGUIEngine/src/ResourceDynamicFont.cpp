#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceDynamicFont.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_RenderManager.h"
#include "EditTextManager.h"
#include "MyGUI_EditText.h"

namespace MyGUI
{

	static const unsigned char FONT_MASK_SELECT = 0x88;
	static const unsigned char FONT_MASK_SELECT_DEACTIVE = 0x60;
	static const unsigned char FONT_MASK_SPACE = 0x00;
	static const unsigned char FONT_MASK_CHAR = 0xFF;
	static const int MIN_FONT_TEXTURE_WIDTH = 256;

	ResourceDynamicFont::ResourceDynamicFont() :
		mTtfSize(0),
		mTtfResolution(0),
		mAntialiasColour(false),
		mDistance(0),
		mSpaceWidth(0),
		mTabWidth(0),
		mCursorWidth(1),
		mSelectionWidth(6),
		mOffsetHeight(0),
		mHeightPix(0),
		mTexture(nullptr)
	{
		mIsFull = false;
		mWidth = 0;
		mHeight = 0;
		mCurrentStyle = IFont::Normal;
	}

	ResourceDynamicFont::~ResourceDynamicFont()
	{
#ifdef MYGUI_USE_FREETYPE
		FT_Done_FreeType(mftLibrary);
#endif

		if (mTexture != nullptr)
		{
			RenderManager::getInstance().destroyTexture(mTexture);
			mTexture = nullptr;
		}
	}
	
	void ResourceDynamicFont::setStyle( IFont::Style _style )
	{
		if( _style < 0 || _style > 3 )
		{
			mCurrentStyle = IFont::Normal;
			return;
		}
		mCurrentStyle = _style;
	}

	GlyphInfo* ResourceDynamicFont::getGlyphInfo(Char _id)
	{
		for (VectorRangeInfo::iterator iter = mVectorRangeInfo[mCurrentStyle].begin();
			iter != mVectorRangeInfo[mCurrentStyle].end(); ++iter)
		{
			GlyphInfo* info = iter->getInfo(_id);
			if (info == nullptr) continue;
			if( info->codePoint == 0 )//��ʾû��װ��
				break;
			return info;
		}
		switch(_id)
		{
		case FontCodeType::Tab:
			return &mTabGlyphInfo;
		case FontCodeType::Space:
			return &mSpaceGlyphInfo;
		case FontCodeType::Selected:
			return &mSelectGlyphInfo;
		case FontCodeType::SelectedBack:
			return &mSelectDeactiveGlyphInfo;
		case FontCodeType::Cursor:
			return &mCursorGlyphInfo;
		default:
			//û�з��ֵı��볢�Լ���
			GlyphInfo* info = addGlyphInfo( _id );
			if( info != nullptr )
			{
				return info;
			}
		}
	}

	/*�����ͻ��ƽ�����ʣ�����������ȷ��uv
	*/
	bool ResourceDynamicFont::newGlyphInfo( GlyphInfo* pgi,Char _id )
	{
		if( mIsFull )return false;

		if( checkHidePointCode(_id) )return false;

		FT_Error ftResult;

		setItalic();

		ftResult = FT_Load_Char( face, _id, FT_LOAD_RENDER );

		if (ftResult)
		{
			// problem loading this glyph, continue
			MYGUI_LOG(Warning, "cannot load character " << _id << " in font " << getResourceName());
			return false;
		}

		setBold( );

		//FT_Int glyph_advance = (face->glyph->advance.x >> 6 );
		//FT_Int glyph_advance = (face->glyph->metrics.width >> 6 );
		FT_Int glyph_advance = face->glyph->bitmap.width;
		//FT_Int glyph_advance = face->glyph->metrics.horiAdvance >> 6;

		unsigned char* buffer = face->glyph->bitmap.buffer;

		if (nullptr == buffer)
		{
			// Yuck, FT didn't detect this but generated a nullptr pointer!
			MYGUI_LOG(Warning, "Freetype returned nullptr for character " << _id << " in font " << getResourceName());
			return false;
		}

		int y_bearnig = mContext.max_bear - ( face->glyph->metrics.horiBearingY >> 6 );

		if ( mContext.finalWidth - 1 < (mContext.length + face->glyph->bitmap.width + mDistance) )
		{
			mContext.height += mContext.max_height + mDistance;
			mContext.length = mDistance;
		}

		/*���������ַ���С�ľ��δ�С����ȫ����������ͼ�񱻴ݻ�
		*/
		/*
		OgreTexture* pTexture = dynamic_cast<OgreTexture*>(mTexture);
		if( !pTexture )
		{
			MYGUI_LOG(Warning, "ResourceDynamicFont can't dynamic_cast mTexture to OgreTexture!" );
			return false;
		}
		*/
		//�������û���㹻�Ŀռ���������ַ�
		if( mContext.height + mContext.max_height + mDistance > mContext.finalHeight )
		{
		/*�������һ�����������Ų����������ڸ߶���������
			MYGUI_LOG(Warning, "Freetype texture not enough space " << _id << " in font " << getResourceName() 
				<< " width:" << mContext.finalWidth << " height:"<< mContext.finalHeight );
			mIsFull = true; //��ֹ������ʾ
			return false;
			*/
			int oldHeight = mContext.finalHeight;
			mContext.finalHeight += mContext.finalHeight;
			mTexture->resize(mContext.finalWidth,mContext.finalHeight);
			//��Ϊ����������Ҫ�������е����͵���������
			relocal( 1.0,
				(float)(oldHeight)/(float)mContext.finalHeight
				);
			//�����������е�ȫ�����ֵ�uv����
			EditTextManager::EnumeratorEditText e=EditTextManager::getInstance().getEnumerator();
			while( e.next() )
			{
				e->update();
			}
		}

		int bearingX = face->glyph->metrics.horiBearingX >> 6;

		//����ͬʱ��������ǰ��ͼ��
		//uint8* imageData = (uint8*)mTexture->lock(TextureUsage::Write);
		uint8* imageData = (uint8*)mTexture->lock(
			mContext.length+bearingX,mContext.height+y_bearnig,
			mContext.length+bearingX+face->glyph->bitmap.width,mContext.height+y_bearnig+face->glyph->bitmap.rows,
			TextureUsage::Write
			);

		if (imageData != nullptr)
		{
			int height = mContext.height;
			int len = mContext.length;
			int pixel_bytes = mContext.pixel_bytes;
			bool rgbaMode = mContext.rgbaMode;

			size_t data_width = mContext.finalWidth * pixel_bytes;
			//size_t data_width = box.rowPitch * pixel_bytes;

			for (int j = 0; j < face->glyph->bitmap.rows; j++ )
			{
				//int row = j + height + y_bearnig;
				//uint8* pDest = &imageData[(row * data_width) + (len + ( face->glyph->metrics.horiBearingX >> 6 )) * pixel_bytes];
				uint8* pDest = &imageData[(j * data_width)];
				for (int k = 0; k < face->glyph->bitmap.width; k++ )
				{
					if (mAntialiasColour) 
						pDest = writeData(pDest, *buffer, *buffer, rgbaMode);
					else
						pDest = writeData(pDest, FONT_MASK_CHAR, *buffer, rgbaMode);
					buffer++;
				}
			}
		}
		else
		{
			MYGUI_LOG(Error, "ResourceDynamicFont, error lock texture, pointer is nullptr");
		}

		mTexture->unlock();

		addGlyph(pgi, _id, 
			mContext.length+bearingX, mContext.height,
			mContext.length +bearingX+ glyph_advance, mContext.height + mContext.max_height, 
			mContext.finalWidth, mContext.finalHeight, 
			mContext.textureAspect, mOffsetHeight,
			glyph_advance,
			(float)bearingX
		);

		mContext.length += (glyph_advance + mDistance);
		return true;
	}

	void ResourceDynamicFont::relocal( float sx,float sy )
	{
		for( int k = 0;k<4;k++ )
		{
			for( VectorRangeInfo::iterator i = mVectorRangeInfo[k].begin();i!=mVectorRangeInfo[k].end();++i )
			{
				for( VectorGlyphInfo::iterator j = i->range.begin();j!=i->range.end();++j )
				{
					reLocalGlyph( *j ,sx,sy );
				}
			}
		}
		reLocalGlyph(mSpaceGlyphInfo,sx,sy);
		reLocalGlyph(mTabGlyphInfo,sx,sy);
		reLocalGlyph(mSelectGlyphInfo,sx,sy);
		reLocalGlyph(mSelectDeactiveGlyphInfo,sx,sy);
		reLocalGlyph(mCursorGlyphInfo,sx,sy);
	}

	void ResourceDynamicFont::reLocalGlyph( GlyphInfo& gi,float sx,float sy)
	{
		gi.uvRect.left *= sx;
		gi.uvRect.top *= sy;
		gi.uvRect.right *= sx;
		gi.uvRect.bottom *= sy;
	}

	//����һ���µ�����
	GlyphInfo* ResourceDynamicFont::addGlyphInfo( Char _id )
	{
		GlyphInfo gi;

		if( !newGlyphInfo( &gi,_id ) )return nullptr;

		//���Ȳ��ҿ�����û�к��ʵ�RangeInfo������оͷ�������
		for( VectorRangeInfo::iterator iter = mVectorRangeInfo[mCurrentStyle].begin();
			iter != mVectorRangeInfo[mCurrentStyle].end(); ++iter)
		{
			if( iter->isExist( _id ) )
			{
				iter->setInfo( _id,&gi );
				return iter->getInfo( _id );
			}
		}
		/*û�з��ֺ��ʵ�RangeInfo�����һ���µ�
			��unicode16�ֳ�ÿ256(0-FF)Ϊһ����Rang��Ȼ���ں͹��еķ�Χ��һ���ų�����
		*/
		Char begin,end;
		begin = _id & 0xff00;
		end = _id | 0xff;
		for( VectorRangeInfo::iterator iter = mVectorRangeInfo[mCurrentStyle].begin(); 
			iter != mVectorRangeInfo[mCurrentStyle].end(); ++iter)
		{
			if( iter->first >= begin && iter->first <= end &&
				iter->last >= begin && iter->last <= end ) //��������begin,end�У���begin,end�ֳ�3������
			{
				if( _id < iter->first )
					end = iter->first-1;
				else
					begin = iter->last+1;
			}
			else
			{
				if( iter->first >= begin && iter->first <= end  )
					end = iter->first-1;
				if( iter->last >= begin && iter->last <= end )
					begin = iter->last+1;
			}
		}
		mVectorRangeInfo[mCurrentStyle].push_back( RangeInfo(begin,end) );
		mVectorRangeInfo[mCurrentStyle].back().setInfo( _id,&gi );
		return mVectorRangeInfo[mCurrentStyle].back().getInfo( _id );
	}

	void ResourceDynamicFont::addGlyph(GlyphInfo* _info, Char _index,
		int _left, int _top, int _right, int _bottom, 
		int _finalw, int _finalh, float _aspect, int _addHeight,
		float _advance,float _bearingX )
	{
		_info->codePoint = _index;
		_info->uvRect.left = (float)_left / (float)_finalw;  // u1
		_info->uvRect.top = (float)(_top + _addHeight) / (float)_finalh;  // v1
		_info->uvRect.right = (float)( _right ) / (float)_finalw; // u2
		_info->uvRect.bottom = ( _bottom + _addHeight ) / (float)_finalh; // v2

		_info->width = (float)(_right - _left);
		_info->height = (float)(_bottom - _top);
		if( _advance == 0.0f )
			_info->advance = _info->width;
		else
			_info->advance = _advance;

		/*
			�����б�壬��Ϊ����ˡ�������Ҫ����ĵ���
		*/
		/*
		if( mCurrentStyle == IFont::Italic||mCurrentStyle == IFont::BoldItalic )
		{
			//���Ȳ��ұ�׼���еĿ��
			
			for( VectorRangeInfo::iterator iter = mVectorRangeInfo[IFont::Normal].begin(); 
				iter != mVectorRangeInfo[IFont::Normal].end(); ++iter)
			{
				GlyphInfo* info = iter->getInfo(_index);
				if( info )
				{
					//+1��΢������б��ľ���
					_info->bearingX = _bearingX - (_info->width-info->width);
					return;
				}
			}
			//+1��΢������б��ľ���
			_info->bearingX = _bearingX;
		}
		else
			*/
			_info->bearingX = _bearingX;
	}

	uint8* ResourceDynamicFont::writeData(uint8* _pDest, unsigned char _luminance, unsigned char _alpha, bool _rgba)
	{
		if (_rgba)
		{
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _alpha; // alpha
		}
		else
		{
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _alpha; // alpha
		}
		return _pDest;
	}

	void ResourceDynamicFont::initialise()
	{

#ifndef MYGUI_USE_FREETYPE

		MYGUI_LOG(Error, "ResourceDynamicFont '" << getResourceName() << "' - Ttf font disabled. Define MYGUI_USE_FREETYE if you need ttf fonts.");

#else // MYGUI_USE_FREETYPE

		/* ��ǰ�ļ��ط�ʽ��ֱ�Ӷ���ȫ���������ļ��������ĸ���ֿ��Կ�����ɡ�
			���Ƕ������ľ���Ч������
		*/
//		IDataStream* datastream = DataManager::getInstance().getData(mSource);
//		if (!datastream)
//		{
//			MYGUI_LOG(Error, "Could not open font face!");
//			return;
//		}

		mTexture = RenderManager::getInstance().createTexture(MyGUI::utility::toString((size_t)this, "_TrueTypeFont"));

		// ManualResourceLoader implementation - load the texture
		// Init freetype
		if ( FT_Init_FreeType( &mftLibrary ) ) MYGUI_EXCEPT("Could not init FreeType library!");

		// Load font
		//FT_Face face;

//		size_t datasize = datastream->size();
//		uint8* data = new uint8[datasize];
//		datastream->read(data, datasize);
//		DataManager::getInstance().freeData(datastream);
		std::string fontFile = DataManager::getInstance().getDataPath(mSource);
		
		/*����ʹ���ļ�ģʽ�����ʹ���ڴ�ģʽ���ò�һֱ����data���ݡ�
		*/
		//if ( FT_New_Memory_Face( mftLibrary, data, (FT_Long)datasize, 0, &face ) )
		if( FT_New_Face( mftLibrary,fontFile.c_str(),0,&face) )
		{
			MYGUI_EXCEPT("Could not open font face!");
		}
		// Convert our point size to freetype 26.6 fixed point format
		FT_F26Dot6 ftSize = (FT_F26Dot6)(mTtfSize * (1 << 6));
		if ( FT_Set_Char_Size( face, ftSize, 0, mTtfResolution, mTtfResolution ) )
			MYGUI_EXCEPT("Could not set char size!");

		int max_height = 0, max_bear = 0;

		int spec_len = mCursorWidth + mSelectionWidth + mSelectionWidth + mSpaceWidth + mTabWidth + (mDistance * 5);
		int len = mDistance + spec_len;
		int height = 0; // �٧է֧�� �ڧ���ݧ�٧�֧��� �ܧѧ� �ܧ�ݧݧڧ�֧��ӧ� ������

		int finalWidth = MIN_FONT_TEXTURE_WIDTH;
		// trying to guess necessary width for texture
		while (mTtfSize * mTtfResolution > finalWidth * 6) finalWidth *= 2;

		for (VectorRangeInfo::iterator iter = mVectorRangeInfo[mCurrentStyle].begin();
			iter != mVectorRangeInfo[mCurrentStyle].end(); ++iter)
		{
			for (Char index = iter->first; index <= iter->last; ++index)
			{

				// ��ڧާӧ�� ��ڧ��ӧѧ�� �ߧ֧ߧ�اߧ�
				if (checkHidePointCode(index)) continue;

				if (FT_Load_Char( face, index, FT_LOAD_RENDER )) continue;
				if (nullptr == face->glyph->bitmap.buffer) continue;
				FT_Int advance = (face->glyph->advance.x >> 6 ) + ( face->glyph->metrics.horiBearingX >> 6 );

				if ( ( 2 * ( face->glyph->bitmap.rows << 6 ) - face->glyph->metrics.horiBearingY ) > max_height )
					max_height = ( 2 * ( face->glyph->bitmap.rows << 6 ) - face->glyph->metrics.horiBearingY );

				if ( face->glyph->metrics.horiBearingY > max_bear )
					max_bear = face->glyph->metrics.horiBearingY;

				len += (advance + mDistance);
				if ( finalWidth - 1 < (len + advance + mDistance) )
				{
					height ++;
					len = mDistance;
				}

			}
		}

		max_height >>= 6;
		max_bear >>= 6;

		int finalHeight = (height + 1) * (max_height + mDistance) + mDistance;

		//make it more squared
		while (finalHeight > finalWidth)
		{
			finalHeight /= 2;
			finalWidth *= 2;
		}

		// �ӧ��ڧ�ݧ�֧� �ҧݧڧاѧۧ��� �ܧ�ѧ�ߧ�� 2
		int needHeight = 1;
		while (needHeight < finalHeight) needHeight <<= 1;
		finalHeight = needHeight;

		/*�ֶ��趨��ߣ�������������������xml�ļ�
			ͬʱ����ߴ�Ҳ���ǹ̶�����ģ�������ӵ��ַ�ʹ���ʲ������ɡ����ʽ��ᱻ�ӳ���
			���������Ա�֤�Ѿ����ڵ��ַ����������µĲ��ʡ�
			������Ĳ��ʳߴ�ĳ�ֵ��512x512���ڿռ䲻����ʱ��������512x1024��
		*/
		if( mWidth != 0 && mHeight != 0 )
		{
			if( finalWidth < mWidth )
				finalWidth = mWidth;
			if( finalHeight < mHeight )
				finalHeight = mHeight;
		}

		float textureAspect = (float)finalWidth / (float)finalHeight;

		// if L8A8 (2 bytes per pixel) not supported use 4 bytes per pixel R8G8B8A8
		// where R == G == B == L
		bool rgbaMode = ! MyGUI::RenderManager::getInstance().isFormatSupported(PixelFormat::L8A8, TextureUsage::Static | TextureUsage::Write);

		const size_t pixel_bytes = rgbaMode ? 4 : 2;
		size_t data_width = finalWidth * pixel_bytes;
		size_t data_size = finalWidth * finalHeight * pixel_bytes;

		MYGUI_LOG(Info, "ResourceDynamicFont '" << getResourceName() << "' using texture size " << finalWidth << " x " << finalHeight);
		MYGUI_LOG(Info, "ResourceDynamicFont '" << getResourceName() << "' using real height " << max_height << " pixels");
		mHeightPix = max_height;

		uint8* imageData = new uint8[data_size];

		uint8* dest = imageData;
		// Reset content (White, transparent)
		for (size_t i = 0; i < data_size; i += pixel_bytes)
		{
			dest = writeData(dest, 0xFF, 0x00, rgbaMode);
		}

		// ��֧ܧ��֧� ���ݧ�ا֧ߧڧ� �� ��֧ܧ�����
		len = mDistance;
		height = mDistance; // �٧է֧�� �ڧ���ݧ�٧�֧��� �ܧѧ� ��֧ܧ��֧� ���ݧ�ا֧ߧڧ� �ӧ�����
		FT_Int advance = 0;

		//------------------------------------------------------------------
		// ���٧էѧ֧� ��ڧާӧ�� ����ҧ֧ݧ�
		//------------------------------------------------------------------
		advance = mSpaceWidth;

		// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
		if ( finalWidth - 1 < (len + advance + mDistance) )
		{
			height += max_height + mDistance;
			len = mDistance;
		}

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SPACE, rgbaMode);
			}
		}

		addGlyph(&mSpaceGlyphInfo, FontCodeType::Space, 
			len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// ���٧էѧ֧� ��ѧҧ�ݧ��ڧ�
		//------------------------------------------------------------------
		advance = mTabWidth;

		// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
		if ( finalWidth - 1 < (len + advance + mDistance) )
		{
			height += max_height + mDistance;
			len = mDistance;
		}

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SPACE, rgbaMode);
			}
		}

		addGlyph(&mTabGlyphInfo, FontCodeType::Tab, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// ���٧էѧ֧� �ӧ�է֧ݧ֧ߧڧ�
		//------------------------------------------------------------------
		advance = mSelectionWidth+2*mDistance;
		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SELECT, rgbaMode);
			}
		}

		// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
		if ( finalWidth - 1 < (len + advance + mDistance) )
		{
			height += max_height + mDistance;
			len = mDistance;
		}

		addGlyph(&mSelectGlyphInfo, FontCodeType::Selected, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// ���٧էѧ֧� �ߧ֧ѧܧ�ڧӧߧ�� �ӧ�է֧ݧ֧ߧڧ�
		//------------------------------------------------------------------
		advance = mSelectionWidth+2*mDistance;

		// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
		if ( finalWidth - 1 < (len + advance + mDistance) )
		{
			height += max_height + mDistance;
			len = mDistance;
		}

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SELECT_DEACTIVE, rgbaMode);
			}
		}

		addGlyph(&mSelectDeactiveGlyphInfo, FontCodeType::SelectedBack, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// ���٧էѧ֧� �ܧ�����
		//------------------------------------------------------------------
		advance = mCursorWidth;

		// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
		if ( finalWidth - 1 < (len + advance + mDistance) )
		{
			height += max_height + mDistance;
			len = mDistance;
		}

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				//pDest = writeData(pDest, (k & 1) ? 0 : 0xFF, FONT_MASK_CHAR, rgbaMode);
				pDest = writeData(pDest, 0 , FONT_MASK_CHAR, rgbaMode);
			}
		}

		addGlyph(&mCursorGlyphInfo, FontCodeType::Cursor, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// ���٧էѧ֧� �ӧ�� ����ѧݧ�ߧ�� ��ڧާӧ�ݧ�
		//------------------------------------------------------------------
		FT_Error ftResult;
		for (VectorRangeInfo::iterator iter = mVectorRangeInfo[mCurrentStyle].begin(); iter != mVectorRangeInfo[mCurrentStyle].end(); ++iter)
		{
			size_t pos = 0;
			for (Char index = iter->first; index <= iter->last; ++index, ++pos)
			{
				// ���ާӧ�� ��ڧ��ӧѧ�� �ߧ� �ߧѧէ�
				if (checkHidePointCode(index)) continue;

				GlyphInfo& info = iter->range.at(pos);

				setItalic();

				ftResult = FT_Load_Char( face, index, FT_LOAD_RENDER );

				if (ftResult)
				{
					// problem loading this glyph, continue
					MYGUI_LOG(Warning, "cannot load character " << index << " in font " << getResourceName());
					continue;
				}

				setBold();

				//FT_Int glyph_advance = (face->glyph->metrics.width >> 6 );
				//FT_Int glyph_advance = face->glyph->advance.x >> 6;
				FT_Int glyph_advance = face->glyph->bitmap.width;

				//FT_Int glyph_advance =  face->glyph->metrics.horiAdvance>>6;
				unsigned char* buffer = face->glyph->bitmap.buffer;

				if (nullptr == buffer)
				{
					// Yuck, FT didn't detect this but generated a nullptr pointer!
					MYGUI_LOG(Warning, "Freetype returned nullptr for character " << index << " in font " << getResourceName());
					continue;
				}

				int y_bearnig = max_bear - ( face->glyph->metrics.horiBearingY >> 6 );

				// ��֧�֧ӧ�� �ߧ� �ߧ�ӧ�� �����ܧ�
				if ( finalWidth - 1 < (len + face->glyph->bitmap.width + mDistance) )
				{
					height += max_height + mDistance;
					len = mDistance;
				}

				int bearingX = face->glyph->metrics.horiBearingX >> 6;
				for (int j = 0; j < face->glyph->bitmap.rows; j++ )
				{
					int row = j + (int)height + y_bearnig;
					uint8* pDest = &imageData[(row * data_width) + (len + bearingX) * pixel_bytes];
					for (int k = 0; k < face->glyph->bitmap.width; k++ )
					{
						if (mAntialiasColour) pDest = writeData(pDest, *buffer, *buffer, rgbaMode);
						else pDest = writeData(pDest, FONT_MASK_CHAR, *buffer, rgbaMode);
						buffer++;
					}
				}

				addGlyph(&info, index, 
					len+bearingX, height, 
					len +bearingX + glyph_advance, height + max_height, 
					finalWidth, finalHeight, textureAspect, mOffsetHeight,
					glyph_advance,
					(float)bearingX
					);
				len += (glyph_advance + mDistance);

			}
		}

		{ //�ռ����������ģ�������µ�����ʱ��Ҫ��Щ��Ϣ
			mContext.length = len;
			mContext.height = height;
			mContext.max_height = max_height;
			mContext.finalHeight = finalHeight;
			mContext.finalWidth = finalWidth;
			mContext.max_bear = max_bear;
			mContext.pixel_bytes = pixel_bytes;
			mContext.advance = advance;
			mContext.textureAspect = textureAspect;
			mContext.rgbaMode = rgbaMode;
		}

		mTexture->createManual(finalWidth, finalHeight, TextureUsage::Static | TextureUsage::Write, rgbaMode ? PixelFormat::R8G8B8A8 : PixelFormat::L8A8);

		void* buffer_ptr = mTexture->lock(TextureUsage::Write);
		if (buffer_ptr != nullptr)
		{
			memcpy(buffer_ptr, imageData, data_size);
		}
		else
		{
			MYGUI_LOG(Error, "ResourceDynamicFont, error lock texture, pointer is nullptr");
		}
		mTexture->unlock();

		delete [] imageData;
//		delete [] data;

#endif // MYGUI_USE_FREETYPE

	}

	void ResourceDynamicFont::setItalic()
	{
		FT_Matrix matrix;
		if( mCurrentStyle == IFont::BoldItalic || mCurrentStyle == IFont::Italic )
		{	//�о������0.3�ı�б��
			matrix.xx = 0x10000L;
			matrix.xy = (FT_Fixed)(0.3*0x10000L);
			matrix.yx = 0;
			matrix.yy = 0x10000L;
		}
		else
		{ //��λ����
			matrix.xx = 0x10000L;
			matrix.xy = 0;
			matrix.yx = 0;
			matrix.yy = 0x10000L;
		}
		FT_Set_Transform(face,&matrix,0);
	}

	void ResourceDynamicFont::setBold()
	{
		FT_Error ftResult;
		if( mCurrentStyle == IFont::BoldItalic || mCurrentStyle == IFont::Bold ) //����Ӵ�
		{
			FT_GlyphSlot_Own_Bitmap(face->glyph);
			ftResult = FT_Bitmap_Embolden( mftLibrary,&face->glyph->bitmap,32,32 );
			if( ftResult )
			{
				MYGUI_LOG(Warning,"FT_Bitmap_Embolden error code:"<<ftResult);
			}
		}
	}

	void ResourceDynamicFont::addCodePointRange(Char _first, Char _second)
	{
		mVectorRangeInfo[mCurrentStyle].push_back(RangeInfo(_first, _second));
	}

	void ResourceDynamicFont::addHideCodePointRange(Char _first, Char _second)
	{
		mVectorHideCodePoint.push_back(PairCodePoint(_first, _second));
	}

	// ����ӧ֧��֧�, �ӧ��էڧ� �ݧ� ��ڧާӧ�� �� �٧�ߧ� �ߧ֧ߧ�اߧ�� ��ڧާӧ�ݧ��
	bool ResourceDynamicFont::checkHidePointCode(Char _id)
	{
		for (VectorPairCodePoint::iterator iter = mVectorHideCodePoint.begin(); iter != mVectorHideCodePoint.end(); ++iter)
		{
			if (iter->isExist(_id)) return true;
		}
		return false;
	}

	void ResourceDynamicFont::clearCodePointRanges()
	{
		for( int i = 0;i < 4;++i )
			mVectorRangeInfo[i].clear();
		mVectorHideCodePoint.clear();
	}

	void ResourceDynamicFont::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Property")
			{
				const std::string& key = node->findAttribute("key");
				const std::string& value = node->findAttribute("value");
				if (key == "Source") mSource = value;
				else if (key == "Size") mTtfSize = utility::parseFloat(value);
				else if (key == "Resolution") mTtfResolution = utility::parseUInt(value);
				else if (key == "Antialias") mAntialiasColour = utility::parseBool(value);
				else if (key == "SpaceWidth") mSpaceWidth = utility::parseInt(value);
				else if (key == "TabWidth") mTabWidth = utility::parseInt(value);
				//else if (key == "CursorWidth") mCursorWidth = utility::parseInt(value);
				else if (key == "Distance") mDistance = utility::parseInt(value);
				else if (key == "OffsetHeight") mOffsetHeight = utility::parseInt(value);
				else if(key == "Width") mWidth = utility::parseInt(value);
				else if(key == "Height") mHeight = utility::parseInt(value);
			}
			else if (node->getName() == "Codes")
			{
				xml::ElementEnumerator range = node->getElementEnumerator();
				while (range.next("Code"))
				{
					std::string range_value;
					std::vector<std::string> parse_range;
					// �էڧѧ�ѧ٧�� �ӧܧݧ��֧ߧڧ�
					if (range->findAttribute("range", range_value))
					{
						parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							int first = utility::parseInt(parse_range[0]);
							int last = parse_range.size() > 1 ? utility::parseInt(parse_range[1]) : first;
							addCodePointRange(first, last);
						}
					}
					// �էڧѧ�ѧ٧�� �ڧ�ܧݧ��֧ߧڧ�
					else if (range->findAttribute("hide", range_value))
					{
						parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							int first = utility::parseInt(parse_range[0]);
							int last = parse_range.size() > 1 ? utility::parseInt(parse_range[1]) : first;
							addHideCodePointRange(first, last);
						}
					}
				}
			}
		}

		// �ڧߧڧ�ڧѧݧڧ٧ڧ��֧�
		initialise();
	}

	ITexture* ResourceDynamicFont::getTextureFont()
	{
		return mTexture;
	}

	// ���ݧ��ڧӧ�ѧ��� �ӧ����� ���� �ԧ֧ߧ֧�ѧ�ڧ� �� ��ڧܧ�֧ݧ��
	int ResourceDynamicFont::getDefaultHeight()
	{
		return mHeightPix;
	}

} // namespace MyGUI
