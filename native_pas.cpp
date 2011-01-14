#include <Winternl.h>
//! RtlInitUnicodeString() probably won't work with wc_str().  Check this.

bool NativeDir(wxString Dir, wxArrayString &List)
{

    UNICODE_STRING UName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hObject;
    ULONG index;
    OBJDIR_INFORMATION *DirObjInformation;
    ULONG dw;
    bool Result = true;

    if (!Setup())
        return false;

    RtlInitUnicodeString(&UName, Dir.wc_str(wxConvLocal));

    InitializeObjectAttributes (
        &ObjectAttributes,
        &UName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL);

    Status = NtOpenDirectoryObject(
        &hObject,
        STANDARD_RIGHTS_READ | DIRECTORY_QUERY,
        &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        printf("NtOpenDirectoryObject = 0x%lX (%S)\n", Status, Dir.c_str());
        return false;
    }
    
    index = 0; // start index
    DirObjInformation = (OBJDIR_INFORMATION*)malloc(1024);
    
    while (1) {
        ZeroMemory(DirObjInformation, 1024);
        Status = NtQueryDirectoryObject(
            hObject,
            DirObjInformation,
            1024,
            TRUE,         // get next index
            FALSE,        // don't ignore index input
            &index,
            &dw);         // can be NULL
    
        if (NT_SUCCESS(Status))
            List.Add(DirObjInformation->ObjectName.Buffer);
        else
            break;
            // printf("NtQueryDirectoryObject = 0x%lX (%S)\n", ntStatus, pszDir);
    }

    //NtClose(hObj);
    CloseHandle(hObject);
    return true;
}

wxString NativeReadLink(wxString Link)
{
    UNICODE_STRING  UName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS    Status;
    HANDLE      hObject;
    ULONG       dw;
    wxString    Result;

    Setup();

    RtlInitUnicodeString(&UName, Link.wc_str(wxConvLocal));

    InitializeObjectAttributes (
        &ObjectAttributes,
        &UName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL);

    Status = NtOpenSymbolicLinkObject(&hObject, SYMBOLIC_LINK_QUERY, &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        UName.Length = 0;
        UName.MaximumLength = 1024;
        UName.Buffer = new WCHAR[1024];

        Status = NtQuerySymbolicLinkObject(hObject, &UName, &dw);

        if (NT_SUCCESS(Status))
            Result = UName.Buffer;
        CloseHandle(hObject);
        delete [] UName.Buffer;
    }
}

//
//void Log(wxString msg)
//{
//   PRINTF("%s\n", msg.c_str());
//}
//
//void EnumerateVolumeMountPoints(wxString Volume, wxArrayString &List, char *Buffer)
//{
//    HANDLE vh = FindFirstVolumeMountPoint(Volume, Buffer, 1024);
//
//    if (vh == INVALID_HANDLE_VALUE)
//        return;
//
//    do {
//        //Log('Mount point = ' + Buffer);
//        List.Add(Buffer + Volume);
//    } while (FindNextVolumeMountPoint(vh, Buffer, 1024));
//    FindVolumeMountPointClose(vh);
//}
//
//void GetListOfMountPoints(wxArrayString &MountPoints, char *Buffer)
//{
//    HANDLE h;
//    LoadVolume();
//
//    // Enumerate the volumes
//
//    h = FindFirstVolume(Buffer, 1024);
//    if (h == INVALID_HANDLE_VALUE)
//        return;
//    do {
//        //Log('FindVolume' + Buffer);
//        EnumerateVolumeMountPoints(Buffer, MountPoints, Buffer);
//    } while (FindNextVolume(h, Buffer, 1024));
//    FindVolumeClose(h);
//}
//
//bool TestDevice(wxString DeviceName, wxString Description)
//{
//    bool Result = false;
//    __int64 Size = 0;
//    DWORD ErrorNo;
//    DISK_GEOMETRY Geometry;
//
//    HANDLE h = NTCreateFile(DeviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
//                            NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);
//    if (h == INVALID_HANDLE_VALUE)
//    {
//        switch (ErrorNo = GetLastError())
//        {
//        case ERROR_FILE_NOT_FOUND: break; // does not matter
//        case ERROR_PATH_NOT_FOUND: break; // does not matter
//        case ERROR_ACCESS_DENIED:
////            MessageDlg('This program requires Administrator privilages to run', mtError, [mbOK], 0);
//            break;
//        case ERROR_SHARING_VIOLATION:
//            // in use (probably mounted)...
//            break;
//        default:
//            printf("Error %d opening device.\n", ErrorNo);
//            return false;
//        }
//    }
//    //Log('Opened ' + DeviceName);
//    Result = true;
//
//    // get the geometry...
//    if (DeviceIoControl(h, CtlCode(FILE_DEVICE_DISK, 0, METHOD_BUFFERED, FILE_ANY_ACCESS),
//                        NULL, 0, &Geometry, sizeof(Geometry), &Len, NULL))
//    {
//        Description = MediaDescription(Geometry.MediaType) +
//                      wxString::Format(". Block size = %d", Geometry.BytesPerSector);
////        Size.QuadPart := Geometry.Cylinders.QuadPart * Geometry.TracksPerCylinder * Geometry.SectorsPerTrack * Geometry.BytesPerSector;
////        Log('size = ' + IntToStr(Size.QuadPart));
//    } else { // DeviceIoControl() failed.
//        Geometry.MediaType = Media_Type_Unknown;
//       //ShowError('reading geometry');
//    }
//    Size = GetSize(h);
//    CloseHandle(h);
//}
//
//void PrintNT4BlockDevices()
//{
//    wxSortedArrayString Devices, Harddisks;
//    wxString Number, DeviceName, Description, VolumeLink;
//    int i, DriveNo, PartNo;
//    DWORD ErrorNo, Len;
//    __int64 Size;
//
//    NativeDir("\\Device", Devices);
//    {
//        for (i = 0; i < Devices.Count(); i++)
//        {
//            DeviceName = "";
//            if (Devices[i].StartsWith("CdRom", &Number))
//                DeviceName = "\\Device\\CdRom" + Number;
//            if (Devices[i].StartsWith("Floppy", &Number))
//                DeviceName = "\\Device\\Floppy%d" + Number;
//            if (Devices[i].StartsWith("Harddisk", &Number))
//                DeviceName = "\\Device\\Harddisk%d" + Number;  // scan the partitions...
//            else
//                continue;
//
//            if (TestDevice(DeviceName, Description))
//            {
//                Log("\\\\?" + DeviceName);
//                VolumeLink = NativeReadLink(DeviceName);
//                if (VolumeLink.Len())
//                    Log("  link to \\\\?" + VolumeLink);
//                if (Description.Len())
//                    Log("  " + Description);
//                if (Size > 0)
//                    Log(wxString::Format("  size is %I64d bytes", Size));
//            }
//        }
//    }
//
//    // do the hard disk partitions...
//    for (DriveNo = 0; DriveNo < Harddisks.Count(); DriveNo++)
//    {
//        Devices.Clear();
//        NativeDir(Harddisks[DriveNo], Devices);
//        for (i = 0; i < Devices.Count(); i++)
//        {
//            if (Devices[i].StartsWith("Partition", &Number))
//            {
//                PartNo = atoi(Number);
//                if (PartNo >= 0)
//                {
//                    DeviceName = Harddisks[DriveNo] + "\\Partition" + Number;
//                    if (TestDevice(DeviceName, Description))
//                    {
//                        Log("\\\\?" + DeviceName);
//                        VolumeLink = NativeReadLink(DeviceName);
//                        if (VolumeLink.Len())
//                            Log("  link to \\\\?" + VolumeLink);
//                        if (Description.Len())
//                            Log("  " + Description);
//                        if (Size > 0)
//                            Log(wxString::Format("  size is %I64d bytes", Size));
//                    }
//                }
//            }
//        }
//    }
//}
//
//void PrintBlockDevices(wxString Filter)
//{
//    HANDLE h;
//    //wxString VolumeName;
//    wxArrayString MountPoints, MountVolumes;
//    wxString VolumeLink;
//    int i;
//    char Drive;
//    wxString DriveString;
//    char Buffer[1024];
//    int MountCount;
//    wxString VolumeLetter[26];
//
//    // search for block devices...
//    Log("Win32 Available Volume Information");
//    LoadVolume();
//    GetListOfMountPoints(MountPoints, Buffer);
//    for (i = 0; i < MountPoints.Count(); i++)
//    {
//        if (GetVolumeNameForVolumeMountPoint(MountPoints[i], Buffer, sizeof(Buffer)))
//            MountVolumes.Add(Buffer);
//        else
//            MountVolumes.Add("");
//
//        // volumes only work on 2k+
//        // for NT4 we need to search physicaldrive stuff.
//
//        for (Drive = 0; Drive < 26; Drive++)
//        {
//            DriveString.Printf("%c:\\", 'a' + Drive);
//            if (GetVolumeNameForVolumeMountPoint(DriveString, Buffer, sizeof(Buffer)))
//            {
//                if (strlen(Buffer))
//                    VolumeLetter[Drive] = Buffer;
//            }
//        }
//
//
//        h = FindFirstVolume(Buffer, sizeof(Buffer));
//        if (h != INVALID_HANDLE_VALUE)
//        {
//            do
//            {
//                wxString VolumeName = Buffer+5;
//                Log("\\\\.\\" + VolumeName);
//                // see where this symlink points...
//                VolumeLink = NativeReadLink("\\??\\" + VolumeName);
//                if (VolumeLink.Len())
//                    Log("  link to \\?" + VolumeLink);
//                Log("  " + GetDriveTypeDescription(GetDriveType(VolumeName)));
//
//                MountCount = 0;
//                // see if this matches a drive letter...
//                for (Drive = 0; Drive < 26; Drive++)
//                {
//                    if (VolumeLetter[Drive] == VolumeName)
//                    {
//                        Log(wxString::Format("  Mounted on \\\\.\\%c:", Drive + 'a'));
//                        MountCount++;
//                    }
//                }
//                
//                // see if this matches a mount point...
//                for (i = 0; i < MountPoints.Count(); i++)
//                {
//                    if (MountVolumes[i] == VolumeName)
//                    {
//                        Log("  Mounted on " + MountPoints[i]);
//                        MountCount++;
//                    }
//                }
//
//                if (MountCount == 0)
//                    Log("  Not mounted");
//
//                Log("");
//            } while (FindNextVolume(h, Buffer, sizeof(Buffer)));
//            FindVolumeClose(h);
//        } // if (h != INVALID_HANDLE_VALUE)
//    } // for each MountPoint
//    Log("");
//        
//    Log("NT Block Device Objects");
//    PrintNT4BlockDevices();
//}
