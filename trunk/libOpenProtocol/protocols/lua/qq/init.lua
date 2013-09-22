-- Generic startup script
print('init.lua initiated with '.._VERSION)
print('MIMQQ4c/2.0 OpenProtocol instance=',OP_inst)

if OP_ua==nil then OP_ua='Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.31 (KHTML, like Gecko) Chrome/26.0.1410.64 Safari/537.31' end

if OP_fontname==nil then
	OP_fontname='å®‹ä½“'
	OP_fontsize=12
	OP_fontcolor='000000'
	OP_fontbold=0
	OP_fontitalic=0
	OP_fontunderline=0
end

if OP_resume==nil then
	-- dofile('lua/qq/comm.lua')
	-- dofile('lua/qq/bit.lua')
	-- dofile('lua/qq/func.lua') // TODO
end

if true then

if OP_resume==nil then
	OP_prestop=false
	JSON=require('JSON')
	dofile('lua/qq/api.lua')
	dofile('lua/qq/get_user_friends2.lua')	
	dofile('lua/qq/login.lua')
	-- dofile('lua/qq/eqq.lua')
else
	OP_PollLoop()
end

else
-- [{"poll_type"."group_web_message","value".{"from_uin".3184390410,"group_code".2472696342,"group_type".1,"msg_id".20651,"msg_id2".518533,"msg_type".45,"reply_ip".176752291,"send_uin".3320937177,"to_uin".431533706,"ver".1,"xml"."<?xml version=\"1.0\" encoding=\"utf-8\"?><d><n t=\"h\" u=\"5089575\" i=\"6\" s=\"1.url.cn/qun/feeds/img/server/g16.png\"/><n t=\"t\" s=\"U+5201EU+06AB‰¹U+4E50\"/><n t=\"b\"/><n t=\"t\" s=\"”’‹àçŒ...-¬ŠãU+00E4U+00BCU+00A0\"/></d>"}}]
print('Test Mode')
--local msg='[["font",{"size".9,"color"."000000","style".[0,0,0],"name"."\\u5B8B\\u4F53"}],["cface",{"name"."2B475214F097CF89EC98A2E159CFEA46.jpg","file_id".551750216,"key"."wYAqPaGD2mpm5V5d","server"."123.138.154.68.443"}]," \\u4F8B\\u5982\\u9019\\u6A23\\u5C31\\u6703\\u5728\\u6309OK\\u6642\\u986F\\u793A\\u4E00\\u500B\\u5C0D\\u8A71\\u6846 "]'
--print(msg)
--local j=JSON.decode(msg)
--print(j[1][2].name)
-- j[1][2].name=EncodeUTF8(j[1][2].name)
-- j[3]=EncodeUTF8(j[3])
--local msg2=JSON.encode(j)
--print(msg2)

--clientid='client_id'
--psessionid='psession_id'
--OP_SendMessage('1234567890\tä¸­æ–‡æ¸¬è©¦')
--CaptureQuotedParts("ptuiCB('0','0','http.//webqq.qq.com/loginproxy.html?login2qq=1&webqq_type=10','0','xxxxx','x\\'xxxxxx');")
-- print(string.gsub("ptuiCB('0','0','http.//webqq.qq.com/loginproxy.html?login2qq=1&webqq_type=10','0','xxxxx','x\'xxxxxx');","\\'","'"))

-- OP_UploadQunImage('D.\\Miranda-IM\\587D08324C50F0E9584ACA35CA4B13F1.JPG')

-- ptwebqq=43e00ad9a8906134f18bd33030971b9b43ec7125db90bfb867e904f34c34c839
-- uin=431533706
-- hash=9692C09B

-- local test=OP_Get("http://tgl.ath.cx/ggg",'http://webqq.qq.com/')
-- local test=OP_Post('http://posttestserver.com/post.php?dump&dir=starkwong','http://webqq.qq.com/','r={"d":1,"c":2}');
-- local test=OP_Post('http://posttestserver.com/post.php?dump&dir=starkwong','http://webqq.qq.com/','',-1,{from='control',f='EQQ.Model.ChatMsg.callbackSendPicGroup',vfwebqq='vfwebqq',custom_face='\tD:\\Miranda-IM\\cat64.jpg'})
local test=OP_Get("http://www.sortchinese.com",'http://webqq.qq.com/')
local test2=OP_GetCookie('PHPSESSID')

print(test)
print('phpsessid='..test2)

-- print('P='..P('431533706','43e00ad9a8906134f18bd33030971b9b43ec7125db90bfb867e904f34c34c839'))
print('End of test')
dddd('ddd')
OP_BadReply('Bla Bla')
end
