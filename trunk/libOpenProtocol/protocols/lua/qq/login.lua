-- https://ui.ptlogin2.qq.com/cgi-bin/login?target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fweb.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20121029001
print('login.lua for comm.js v20130810')

g_referer_top='https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20130723001'
g_referer_web2='http://web2.qq.com/'
g_referer_cgi='http://cgi.web2.qq.com/proxy.html?v=20110412001&callback=1&id=1'

pgv_pvid=math.random(1000000000,9999999999)
ssid="s"..math.random(1000000000,9999999999)
-- pgv_pvid=math.random(10000,99999)..math.random(00000,99999)
-- ssid="s"..math.random(10000,99999)..math.random(00000,99999)

-- Utilities
encodeURIComponent=function(s)
	return s
end -- encodeURIComponent

local loadvar=function(s,var)
	local pos=s:find('var '..var)
	local pos2
	-- local ch
	local term=';'
	
	if pos==nil then
		print('login.getvar('..var..'): Value not found!')
		return ''
	else
		pos=pos+4 -- +var:len()
		
		local pos2=s:find(term,pos)
		local str=s:sub(pos,pos2-1)
		
		loadstring(str) -- s:sub(pos,pos2-1)
	end
end

local loadScript=function(url)
	print ('login.loadScript: url='..url)
	local data=OP_Get(url,referer)
	print ('login.loadScript: data='..data)
	data=data:gsub("%);",")")
	loadstring(data)
end

local ptui_needVC=function(B,C)
	-- cookie="pgv_pvid="..pgv_pvid.."; pgv_info=pgvReferrer=&ssid="..ssid.."; ptui_loginuin="..OP_uin.."; chkuin="..B
	cookie="pgv_pvid="..pgv_pvid.."; pgv_info=pgvReferrer=&ssid="..ssid.."; chkuin="..B
	referer=g_referer_top

	local A = "https://ssl.ptlogin2."..g_domain.."/check?"
	if g_regmaster == '2' then
		A = "http://check.ptlogin2.function."..g_domain.."/check?regmaster=2&"
	else
		if g_regmaster == '3' then
			A = "http://check.ptlogin2.crm2."..g_domain.."/check?regmaster=3&"
		end
	end
	A=A.."uin="..B.."&appid="..C.."&js_ver="..g_pt_version.."&js_type=0&login_sig="..g_login_sig.."&u1=http%3A%2F%2Fwebqq.qq.com%2Floginproxy.html&r="..math.random()
	loadScript(A)
end

local check=function()
	g_uin=OP_uin
	ptui_needVC(g_uin, g_appid)
end

-- Entry point

local init=function()
	print('login.init()')
	local	data=OP_Get(g_referer_top,g_referer_web2);
	loadvar(data,'g_version')
	loadvar(data,'g_pt_version')
	loadvar(data,'g_href')
	loadvar(data,'g_appid')
	loadvar(data,'g_daid')
	loadvar(data,'g_regmaster')
	loadvar(data,'g_login_sig')
	loadvar(data,'g_lang')
	loadvar(data,'g_domain')
	loadvar(data,'g_mibao_css')
	encodeURIComponent=nil
	
	print(' g_version='..g_version)
	print(' g_pt_version='..g_pt_version)
	print(' g_appid='..g_appid)
	print(' g_regmaster='..g_regmaster)
	print(' g_login_sig='..g_login_sig)
	print(' g_lang='..g_lang)
	print(' g_domain='..g_domain)
	print(' g_mibao_css='..g_mibao_css)
	
	loadScript('https://ui.ptlogin2.qq.com/cgi-bin/ver',g_referer_top)
	
	login2token=nil
	
	check()
	if verifycode~=nil and verifycode~='' then
		print('Login '..g_uin..' with verifycode '..verifycode)
		ajax_Submit()
	else
		error('ERRLOGNVerify Code Cancelled')
	end
end

-- Delay loads

local loadVC=function(A)
	if A==true then
		verifycode=' '
		while verifycode==' ' do
			print('Requesting verycode')
			local E = "https://ssl.captcha."..g_domain.."/getimage?aid="..g_appid.."&r="..math.random().."&uin="..g_uin
			local tempfile=OP_GetTempFile()
			os.remove(tempfile)
			tempfile=tempfile..'.jpg'
			local img=OP_Get(E,referer)
			if img:len() == 0 then
				print('Verycode request failed, try again')
				img=OP_Get(E,referer)
			end
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
			verifycode=OP_VeryCode(tempfile)
			os.remove(tempfile)
		end
	end
end

ptuiV=function(v)
	print('login.ptuiV: Web2 version '..v)
end

ptui_checkVC=function(B,E,C)
	print('login.ptui_checkVC()')
	
	if C==nil then
		check()
		return
	end

	if C=="\x00\x00\x00\x00\x00\x00\x27\x10" then
			g_uin = "0"
			error('ERRLOGNInvalid UIN')
			return
	end
	
	checkVC_3 = C
	
	if B == "0" then
		verifycode = E
		loadVC(false)
	else
		if B == "1" then
			loadVC(true)
		end
	end
end

local getSubmitUrl=function(K,U,P,V)
	local E = true
	local A = 'https://ssl.ptlogin2.'..g_domain..'/'..K..'?'
	if g_regmaster=='2' then
		A = 'http://ptlogin2.function.'..g_domain..'/'..K..'?regmaster=2&'
	else
		if g_regmaster=='3' then
			A = 'http://ptlogin2.crm2.'..g_domain..'/'..K..'?regmaster=3&'
		end
	end
	
	A=A..'u='..U..'&p='..P..'&verifycode='..V
	A=A..'&webqq_type=10&remember_uin=1&login2qq=1&aid='..g_appid
	A=A..'&u1=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html%3Flogin2qq%3D1%26webqq_type%3D10'
	A=A..'&h=1&ptredirect=0&ptlang='..g_lang..'&daid=164&from_ui=1&pttype=1&dumy=&fp=loginerroralert&action=2-25-154876'
	A=A..'&mibao_css='..g_mibao_css..'&t=1&g=1&js_type=0&js_ver='..g_pt_version..'&login_sig='..g_login_sig
	return A
end

ajax_Submit=function()
	print('login.ajax_Submit: checkVC_3='..checkVC_3)
	
	-- vc2=''
	-- for c=3,string.len(checkVC_3),4 do
	--	vc2=vc2..string.char(tonumber(string.sub(checkVC_3,c,c+1),16))
	-- end
	local pwd_I=OP_MD5(OP_password)
	local pwd_H=OP_MD5(pwd_I..checkVC_3,true)
	local pwd_G=OP_MD5(pwd_H..verifycode,true)

	local A = getSubmitUrl('login',g_uin,pwd_G,verifycode)
	-- pt.winName.set("login_param", encodeURIComponent(login_param));
	loadScript(A)
end

function url_decode(str)
  str = string.gsub (str, "+", " ")
  str = string.gsub (str, "%%(%x%x)",
      function(h) return string.char(tonumber(h,16)) end)
  str = string.gsub (str, "\r\n", "\n")
  return str
end

ptuiCB=function(J, K, B, H, C, A)
	-- ptuiCB('3','0','','0','您输入的帐号或密码不正确，请重新输入。', '431533706');
	-- ptuiCB('0','0','http://webqq.qq.com/loginproxy.html?login2qq=1&webqq_type=10','0','...............', '^_^2');
	-- H=redirection window
	-- J=65~67=qrlogin related
	
	if J == '0' then
		-- Login success
		print('login.ptuiCB: Welcome to MIMQQ4c, '..A..'! :)\n')
		login2token=C
		
		cookie="pgv_pvid="..pgv_pvid.."; pgv_info=pgvReferrer=&ssid="..ssid.."; ptui_loginuin="..g_uin

		if B ~= '' then
			print('login.ptuiCB: Loading redirect '..B)
			OP_Get(B,'https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20130723001')
			
			-- local pos=B:find('?u1=')
			-- local url=url_decode(B:sub(pos+4))
			-- print('#2: '.. url)
			-- OP_Get(url,'https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=5&mibao_css=m_webqq&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fweb2.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20130723001')
			
		end
		
		OP_Post('http://cgi.web2.qq.com/keycgi/qqweb/newuac/get.do',g_referer_cgi,'r='..url_encode(JSON:encode({appid=50,itemlist={'width','height','defaultMode'}}))..'&uin='..g_uin)
		
		return
	else
		print('login.ptuiCB: Login Failed - '..C)
		error('ERRLOGN'..C)
	end
end

init()

init=nil
getvar=nil
loadScript=nil
ptui_needVC=nil
check=nil
loadVC=nil
ptui_checkVC=nil
ptuiV=nil
getSubmitUrl=nil
ajax_Submit=nil
ptuiCB=nil
checkVC_3=nil
verifycode=nil
OP_uin=nil
OP_password=nil

if login2token~=nil then
	print('End of login.lua, handover to eqq.lua')
	-- dofile('lua/qq/eqq.lua')
	dofile('lua/qq/eqq_tmp.lua')
end
