//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "pdriveNT.h"
#include "physicaldrive.h"

PartitionInfo* SelectedPartitionInfo = NULL;
static PList PartitionInfoList;
#define ENUMERATE(l,c,o) for(c* o=(c*)((PList*)(l))->m_pHead;o;o=(c*)((PNode*)o)->m_pNext)

int main()
{
    IPhysicalDrive *Drive = CreatePhysicalDriveInstance();
    //GetDriveNameDialog( GetModuleHandle(NULL), NULL );

    SelectedPartitionInfo = NULL;
    if( PartitionInfoList.IsEmpty() )
    {
        Drive->GetPartitionInfo(&PartitionInfoList);
    }
    ENUMERATE(&PartitionInfoList, PartitionInfo, pi)
    {
        //int iIndex = SendMessage(hListbox,LB_ADDSTRING,0,(LPARAM) (LPCSTR) pi->GetNameAsString());
        //SendMessage(hListbox,LB_SETITEMDATA,iIndex,(LPARAM)pi);
        if (pi->m_PartitionLength)
        {
            printf("%s %s\n", (LPCSTR)pi->GetNameAsString(), (LPCSTR)pi->GetTypeAsString());
            printf(" %15I64d %15I64d\n", pi->m_StartingOffset, pi->m_StartingSector);
        }
    }

    return 0;
}
