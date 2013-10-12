/* Ϊ�˶Զ�̬����Ĳ���ʵ�ֶ�̬����չ����Ҫ�����������չ�󡣵����Ѿ����ڵ�����uv����
	MyGUI�����ִ���������EditText���С���������������չʱ��Ҫ֪ͨȫ����EditTextʵ��
	����uv��������ڱ����м���EditTextManager.h��EditTextManager.cpp�⻹��Ҫ��MyGUI����
	���ֽ����޸ģ��޸Ĳ������£�
	a.�޸�MyGUI_Gui.h�ļ�
		��������Gui�м���
		EditTextManager* mEditTextManager;
		ͬʱ��ǣ�������ļ�MyGUI_Prerequest.h�м���������
		class EditTextManager;
	b.�޸�MyGUI_Gui.cpp�ڶ�Ӧλ�ü���
		mEditTextManager = new EditTextManager();

		mEditTextManager->initialise();
		��
		mEditTextManager->shutdown();

		delete mEditTextManager;
	c.�޸�MyGUI_EditText.h��MyGUI_EditText.cpp
		����EditText�Ĺ�������������зֱ����
		EditTextManager::getInstance().add( this );
		��
		EditTextManager::getInstance().remove( this );
		ͬʱΪEditText�����µķ���void update();�ͱ���bool mNeedUpdate;\

		void EditText::update()
		{
			if (nullptr != mNode) mNode->outOfDate(mRenderItem);

			mNeedUpdate = true;
		}

		�޸ĺ���doRender�Ŀ�ͷ���֣��޸ĺ�����
		if (nullptr == mFont) return;

		if (!mVisible || mEmptyView) 
		{
			if( mNeedUpdate )
				updateRawData();
			mNeedUpdate = false;
			return;
		}

		bool _update = mRenderItem->getCurrentUpdate();
		if (_update || mNeedUpdate ) mTextOutDate = true;

		mNeedUpdate = false;

		if (mTextOutDate) updateRawData();

		Vertex* _vertex = mRenderItem->getCurrentVertexBuffer();
*/
#ifndef __MYGUI_EDITTEXT_MANAGER_H__
#define __MYGUI_EDITTEXT_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Singleton.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_Enumerator.h"

namespace MyGUI
{
	class EditText;

	class MYGUI_EXPORT EditTextManager :
		public Singleton<EditTextManager>
	{
	public:
		typedef std::list<EditText*> EditTextList;
		typedef Enumerator<EditTextList> EnumeratorEditText;
	public:
		EditTextManager();

		void initialise();
		void shutdown();

		void add( EditText* pitem );
		void remove( EditText* pitem );

		EnumeratorEditText getEnumerator() const;
	private:
		EditTextList mEditTexts;
	};
}

#endif