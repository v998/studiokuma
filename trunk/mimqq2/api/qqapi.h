#pragma message("+ libeva")
#pragma warning(disable:4520) // evalogin.h multiple default constructors
#include "libeva.h"
#pragma warning(default:4520)
#include "libuh/evauhpacket.h"
#include "libuh/evauhprotocols.h"

#pragma message("+ EVA API")
#include "evahtmlparser.h"
#include "evaqtutil.h"
#include "evautil.h"
#include "evaipaddress.h"
#include "evaipseeker.h"

#pragma message("+ MIMQQ2 API")
#include "socket.h"
#include "network.h"
#include "userhead.h"
#include "qunimageserver.h"
#include "qunimage.h"
#include "EvaAccountSwitcher.h"

/*
#pragma message("qqapi.evafiledownloader")
#include "filetrans/evafiledownloader.h"
#pragma message("qqapi.evafilemanager")
#include "filetrans/evafilemanager.h"
#pragma message("qqapi.evacachedfile")
#include "filetrans/evacachedfile.h"
#pragma message("qqapi.evaftprotocols")
#include "libft/evaftprotocols.h" // This is in libeva! Take care!
*/
