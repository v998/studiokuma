-- We should follow the protocol to access other pages first, but save some bandwidth now :)

local url='http://check.ptlogin2.qq.com/check?uin='..OP_uin..'&appid=1003903&r='..math.random()
local referer='http://ui.ptlogin2.qq.com/cgi-bin/login?target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fwebqq.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20120920001'
--local referer='http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=3'

pgv_pvid=math.random(1000000000,9999999999)
ssid="s"..math.random(1000000000,9999999999)
cookie=''
-- cookie="pgv_pvid="..pgv_pvid.."; pgv_info=pgvReferrer=&ssid="..ssid.."; chkuin="..OP_uin
-- OP_Get('http://ui.ptlogin2.qq.com/cgi-bin/login?target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fwebqq.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20120619001','http://webqq.qq.com/',cookie)

verycode=nil
print('Initial login check...')
local resp=OP_Get(url,referer,cookie)
if string.find(resp,"ptui_checkVC(",1,true)==nil then
	print('First time request failed for checkVC, try again')
	resp=OP_Get(url,referer,cookie)
end
-- ptui_checkVC('0','!53U','\x00\x00\x00\x00\x19\xb8\xae\x8a');

local ret

if string.find(resp,"ptui_checkVC(",1,true)==nil then error('ERRRESP') end
ret=CaptureQuotedParts(resp)

if ret[1]=='0' then
	verycode=ret[2]
	
	print('Login '..OP_uin..' with verycode '..verycode)
else
	verycode=' '
	while verycode==' ' do
		print('Requesting verycode')
		local url='http://captcha.qq.com/getimage?aid=1003903&r='..math.random()..'&uin='..OP_uin
		local tempfile=OP_GetTempFile()
		os.remove(tempfile)
		tempfile=tempfile..'.jpg'
		local img=OP_Get(url,referer)
		if img:len() == 0 then
			print('Verycode request failed, try again')
			img=OP_Get(url,referer)
		end
		local fh=io.open(tempfile,"wb")
		fh:write(img)
		fh:close()
		verycode=OP_VeryCode(tempfile)
		os.remove(tempfile)
	end
end

if verycode~=nil then
	vc2=''
	for c=3,string.len(ret[3]),4 do
		vc2=vc2..string.char(tonumber(string.sub(ret[3],c,c+1),16))
	end
	local pwd_I=OP_MD5(OP_password)
	local pwd_H=OP_MD5(pwd_I..vc2,true)
	local pwd_G=OP_MD5(pwd_H..verycode,true)
	
	url='http://ptlogin2.qq.com/login?u='..OP_uin..'&p='..pwd_G..'&verifycode='..verycode..'&webqq_type=10&remember_uin=1&login2qq=1&aid=1003903&u1=http%3A%2F%2Fwebqq.qq.com%2Floginproxy.html%3Flogin2qq%3D1%26webqq_type%3D10&h=1&ptredirect=0&ptlang=2052&from_ui=1&pttype=1&dumy=&fp=loginerroralert&action=1-23-14531&mibao_css=m_webqq&t=1&g=1'
	referer='http://ui.ptlogin2.qq.com/cgi-bin/login?target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fwebqq.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20120619001'
	
	--print("url="..url)
	--error('ERRASRT')
	--cookie=cookie.."; ptui_loginuin="..OP_uin
	--OP_Sleep(2000);
	
	print('Logging in...')
	resp=OP_Get(url,referer,cookie)
	
	if string.find(resp,"ptuiCB(",1,true)==nil then
		print('First time failed, try again')
		resp=OP_Get(url,referer,cookie)
	end
	-- ptuiCB('0','0','http://webqq.qq.com/loginproxy.html?login2qq=1&webqq_type=10','0','...............', '^_^2');
	
	print('ASSERT: resp='..resp)
	if string.find(resp,"ptuiCB(",1,true)==nil then error('ERRRESP'..resp) end
	ret=CaptureQuotedParts(resp)
	
	if ret[1]=='0' then
		nick=ret[6]
		print('Welcome to MIMQQ4c, '..nick..'! :)\n')
		
		clientid=math.random(10000000,99999999)
		
		-- login2 is the only channel call that uses POST
		local str='r='..JSON:encode({status=StatusMIM2Text(OP_status),ptwebqq=ptwebqq,passwd_sig='',clientid=clientid})
		url='http://d.web2.qq.com/channel/login2'
		referer='http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=2'
		print('Logging in channel...')
		resp=OP_Post(url,referer,str)
		
		if resp:find('retcode')==nil or resp:byte(resp:len())~=125 then
			print('login2: First time data failed, try again')
			resp=OP_Post(url,referer,str)
		end
		
		local j=JSON:decode(resp)
		if (j.retcode==0) then
			OP_LoginSuccess()
			vfwebqq=j.result.vfwebqq
			psessionid=j.result.psessionid
			
			print('Performing post login actions...')
			API_GetSingleLongNick2(OP_uin)
			API_GetQQLevel2(OP_uin)
			API_GetFriendInfo2(OP_uin)
			FACE_GetFace(OP_uin,1,0)
			API_GetUserFriends2()
			Channel_GetOnlineBuddies2()
			API_GetGroupNameListMask2()
			Channel_GetDiscuListNew2()
			
			OP_resume=true
			OP_PollLoop()
		else
			error('ERRLOG2')
		end
	else
		error('ERRLOGN'..ret[5])
	end
else
	print('No verycode given, cancel operation')
	error('ERRNOVC')
end