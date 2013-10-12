#ifndef __MYGUI_TEXT_TAG_H__
#define __MYGUI_TEXT_TAG_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_IFont.h"

namespace MyGUI
{
	/*
		���Էֽ�Tag:<I>,<B>,<N>,<M>
		����ɹ�����true,result���ö�Ӧ��IFont::Style
		ʧ�ܷ���false,index�ָ���ԭ����λ��
	*/
	template<typename T,typename N> bool tryFontStyleTag( T& index,N& end,IFont::Style& result )
	{
		Char character = *index;

		if( character == L'<' )
		{
			bool bBadTag = false;
			++ index;
			if (index == end )
			{
				--index;
				return false;
			}
			character = *index;

			switch( character )
			{
				case L'I':
					result = IFont::Italic;
					break;
				case L'B':
					result = IFont::Bold;
					break;
				case L'N':
					result = IFont::Normal;
					break;
				case L'M':
					result = IFont::BoldItalic;
					break;
				default:
					--index;//�ָ�����һ��
					character = *index;
					bBadTag = true;
			}
			if( !bBadTag )
			{
				++index;
				if(index == end )
				{
					--index;
					--index;
					return false;
				}
				character = *index;
				if( character == L'>' )
				{
					return true;
				}
				--index;
				--index;
			}
		}
		return false;
	}

	template<typename T,typename N> bool tryColourTag( T& index,N& end,uint32& result )
	{
		// �ާѧ��ڧ� �էݧ� �ҧ������ �ܧ�ߧӧ֧��ѧ�ڧ� ��ӧ֧���
		static const char convert_colour[64] =
		{
			0,  1,  2,  3,  4,  5,  6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
			-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1,  0,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1
		};
		Char character = *index;

		char _c = convert_colour[(character - 48) & 0x3F];
		if( _c >= 0 && character>=48 )
		{
			bool bBadTag = false;
			uint32 colour = _c;
			int retcount = 0;
			// �� �֧�� ����� ��ڧާӧ�ݧ�� ����ݧ� ��ѧ���
			for (char i = 0; i < 5; i++)
			{
				++ index;
				retcount ++;
				if (index == end)
				{
					for( char j = 0; j < retcount;j++ )
						--index;
					return false;
				}
				colour <<= 4;
				character = *index;
				_c = convert_colour[ (character - 48) & 0x3F ];
				if( _c < 0 && character>=48 )
					bBadTag = true;
				colour += _c;
			}

			if( !bBadTag )
			{
				// �֧�ݧ� �ߧ�اߧ�, ��� �ާ֧ߧ�֧� �ܧ�ѧ�ߧ�� �� ��ڧߧڧ� �ܧ�ާ��ߧ֧ߧ��
				result = colour;
				return true;
			}
			//����һ����ɫ���ָ���ȥ���ַ�����
			--index;
			--index;
			--index;
			--index;
			--index;
		}
		return false;
	}
}

#endif