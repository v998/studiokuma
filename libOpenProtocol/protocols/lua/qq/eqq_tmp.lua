print('eqq.lua for eqq.js v20130515')

referer_d='http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=3'
referer_s='http://s.web2.qq.com/proxy.html?v=20110412001&callback=1&id=1'
referer_w='http://web2.qq.com/'
CONN_SERVER_DOMAIN="http://d.web2.qq.com/"
CONN_SERVER_DOMAIN2="http://d.web2.qq.com/"
API_SERVER_URL="http://s.web2.qq.com/api/"
AVATAR_SERVER_DOMAINS={
	"http://face1.web.qq.com/",
	"http://face2.web.qq.com/",
	"http://face3.web.qq.com/",
	"http://face4.web.qq.com/",
	"http://face5.web.qq.com/",
	"http://face6.web.qq.com/",
	"http://face7.web.qq.com/",
	"http://face8.web.qq.com/",
	"http://face9.web.qq.com/"
}
m=CONN_SERVER_DOMAIN
p=0 -- Secret Key

-- Legacy

-- Referers required for each tunnel
OP_referer_s='http://s.web2.qq.com/proxy.html?v=20110412001&callback=1&id=3'
OP_referer_d='http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=2'
OP_referer_g='http://web2.qq.com'

-- State management
quninfovalid={} -- Qun Information receive
sminfovalid={}  -- Session Message Information receive
msgpktbuffer={} -- Duplicated message checking
smindex=0       -- Session Message index (1 based)
qunextid={}     -- Qun real external ID for qun image retrieval
cfacekeytime=0

-- Get 10-characters timestamp
function GetTS()
	return os.time(os.date('*t'))..math.random(100,999)
end

-- Calling API functions with GET method
function CallAPI(func,extra)
	local url='http://s.web2.qq.com/api/'..func..'?'..extra..'&vfwebqq='..vfwebqq..'&t='..GetTS()
	local s=OP_Get(url,OP_referer_s)
	
	print('CallAPI: func='..func)
	-- print('in: '..url)
	-- print('out: '..s)
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		-- not 125, should be 10
		print('ERRRESPCallAPI: First time data failed for '..func..', try again') -- . ref='..s:byte(s:len()))
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Get(url,OP_referer_s)
	end
	
	local j=JSON:decode(s)
	
	if j~=nil and (j.retcode>=0 and j.retcode<1000) then
		return j.result
	else
		print('CallAPI: Bad reply: '..s)
		OP_BadReply(func)
		return nil
	end
end

-- Calling API functions with POST method
function CallAPIPost(func,j1,v,raw)
	local url='http://s.web2.qq.com/api/'..func
	local r
	
	if type(j1)=='table' then
		j1['vfwebqq']=vfwebqq
		r='r='..url_encode(JSON:encode(j1))
	else
		r=url_encode(j1..'&vfwebqq='..vfwebqq)
	end
	
	local s=OP_Post(url,OP_referer_s,r)
	-- if v~=nil then
		print('CallAPIPost: func='..func)
		-- print('CallAPIPost: '..r)
		-- print('in: '..r)
		-- print('out: '..s)
	-- end
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		print('ERRRESPCallAPIPost: First time data failed for '..func..', try again. ref='..s:byte(s:len()))
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Post(url,OP_referer_s,r)
	end
	
	if v~=nil then
		print('CallAPIPost: s='..s)
	end
	
	local j=JSON:decode(s)
	
	if raw~=nil then
		return j
	else
		if j~=nil and j.retcode==0 then
			return j.result
		else
			print('CallAPIPost('..func..') failed, response='..s);
			OP_BadReply(func)
			return nil
		end
	end
end

-- Calling Channel function with GET method
function CallChannel(func,extra)
	local url='http://d.web2.qq.com/channel/'..func..'?'..extra..'clientid='..clientid..'&psessionid='..psessionid..'&t='..GetTS()
	local s=OP_Get(url,OP_referer_d)
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		print('ERRRESPCallChannel: First time data failed for '..func..', try again. ref='..s:byte(s:len()))
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Get(url,OP_referer_d)
	end
	
	local j=JSON:decode(s)
	
	if j~=nil and j.retcode==0 then
		return j.result
	else
		print('return: '..s)
		OP_BadReply(func)
		return nil
	end
end

-- Map QQ status numbers to text
function StatusQQ2Text(st)
	local t={
		[10]="online",
		[20]="offline",
		[30]="away",
		[40]="hidden",
		[50]="busy",
		[60]="callme",
		[70]="silent"
	}

	local ret=t[st]
	if (ret==nil) then ret=t[70] end
	
	return ret
end

-- Map QQ status text to QQ status
function StatusText2QQ(st)
	local t={
		online=10,
		offline=20,
		away=30,
		hidden=40,
		busy=50,
		callme=60,
		silent=70
	}

	local ret=t[st]
	if (ret==nil) then ret=t.online end
	
	return ret
end

-- Map QQ status text to Miranda IM status
function StatusText2MIM(st)
	local t={
		online=40072,  -- ID_STATUS_ONLINE
		offline=40071, -- ID_STATUS_OFFLINE
		away=40073,    -- ID_STATUS_AWAY
		hidden=40078,  -- ID_STATUS_INVISIBLE
		busy=40076,    -- ID_STATUS_OCCUPIED
		callme=40077,  -- ID_STATUS_FREECHAT
		silent=40074   -- ID_STATUS_DND
	}

	local ret=t[st]
	if (ret==nil) then ret=t.online end
	
	return ret
end

-- Map Miranda IM status to QQ status text
function StatusMIM2Text(st)
	local t={
		['40072']='online',  -- ID_STATUS_ONLINE
		['40071']='offline', -- ID_STATUS_OFFLINE
		['40073']='away',    -- ID_STATUS_AWAY
		['40078']='hidden',  -- ID_STATUS_INVISIBLE
		['40076']='busy',    -- ID_STATUS_OCCUPIED
		['40077']='callme',  -- ID_STATUS_FREECHAT
		['40074']='silent'   -- ID_STATUS_DND
	}

	local ret=t[tostring(st)]
	if (st==0) then ret='offline' end
	if (ret==nil) then ret=t['40072'] end
	
	return ret
end

-- Shorthand for mapping Miranda IM status to QQ status number
function StatusMIM2QQ(st)
	return StatusText2QQ(StatusMIM2Text(st))
end

-- Process all kind of IM messages
function ProcessMessage(msg,tuin,gid,msgid,tm)
	local s=''
	local prefix,suffix='',''
	for k2,v2 in ipairs(msg) do
		-- ["font",{"size":9,"color":"000000","style":[0,0,0],"name":"\\u5B8B\\u4F53"}],"b\\u7AD9\\u53BB\\u770B\\u770B\\uFF1F "
		if type(v2)=='table' then
			-- font/cface
			if v2[1]=='font' then
				-- {"size":9,"color":"000000","style":[0,0,0],"name":"\\u5B8B\\u4F53"}
				for k3,v3 in pairs(v2[2]) do
					if k3=='size' then
						prefix=prefix..'[size='..math.floor(v3*OP_pxy/72)..']'
						suffix='[/size]'..suffix
					elseif k3=='color' then
						prefix=prefix..'[color='..v3..']'
						suffix='[/color]'..suffix
					elseif k3=='style' then
						if v3[1]==1 then prefix=prefix..'[b]' suffix='[/b]'..suffix end
						if v3[2]==1 then prefix=prefix..'[i]' suffix='[/i]'..suffix end
						if v3[3]==1 then prefix=prefix..'[u]' suffix='[/u]'..suffix end
					elseif k3=='name' then
						-- MIM does not support custom font yet
					end
				end
			elseif v2[1]=='cface' then
				-- {"name":"{B9F897A5-9393-4DEB-9C7A-D5D277E2542B}.jpg","file_id":190251408,"key":"rU3vuf3rhECtAz9D","server":"123.138.154.68:8000"}
				-- ["cface","5E10B9BCA2BDF8489CB3239DFFA745B5.jpg",""]
				local o2=v2[2]
				-- http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid=1058984205&uin=1606177908&rip=123.138.154.68&rport=8000&fid=2428766877&pic=%7B0FEA5AF3-97B8-48C5-97F8-42FE53D7E6FB%7D.jpg&vfwebqq=1aa616dedcc8f8214e83b28704960dd2233cae8f3c800980eea50e01e93768e8b32af25cffe14342&t=1343645038
				if OP_qunimagedir~=nil then
					if gid~=0 then
						-- Qun Image
						-- key unused
						s=s..'[img]http://127.0.0.1:'..OP_imgserverport..'/qunimages/'..OP_uin..'/'..gid..'/'..tuin..'/'..tm..'/'..string.gsub(o2.server,':','/')..'/'..o2.file_id..'/'..o2.name..'[/img]'
					else
						s=s..'[img]http://127.0.0.1:'..OP_imgserverport..'/p2pimages/'..OP_uin..'/'..tuin..'/'..msgid..'/'..o2..'[/img]'
					end
				else
					s=s..'[image]'
				end
			elseif v2[1]=='offpic' then
				-- Another way of p2p image
				-- ["offpic",{"success":1,"file_path":"/0fb2dca1-72bf-4fce-9da8-3a34f7b2d217"}]," "]
				local o2=v2[2]
				if OP_qunimagedir~=nil and o2.success==1 then
					-- NOTE! There is a padding / before file_path!
					s=s..'[img]http://127.0.0.1:'..OP_imgserverport..'/p2pimages/'..OP_uin..'/'..tuin..o2.file_path..'.jpg[/img]'
				else
					s=s..'[image]'
				end
			elseif v2[1]=='face' then
				-- emoticon
				s=s..'[face:'..v2[2]..']'
			else
				-- unknown
				s=s..v2[1]..'='..JSON:encode(v2[2])
			end
		else
			-- string
			s=s..v2
		end
	end
	if OP_bbcode==true then s=prefix..s..suffix end
	return s
end

-- OP Web Server calls this to fetch P2P Images
function HandleP2PImage(uri)
	-- tuin/msgid/name
	-- tuin/filename
	if clientid==nil or psessionid==nil then
		print("HandleP2PImage: clientid or psessionid not set!")
		return
	end

	local parts=Split(uri,"/")
	if #parts==3 then
		-- http://d.web2.qq.com/channel/get_cface2?lcid=18631&guid=5E10B9BCA2BDF8489CB3239DFFA745B5.jpg&to=3291940000&count=5&time=1&clientid=42759885&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000049500001266016e04008aaeb8196d0000000a40504c4847507a4266466d00000028f2ed4db4ddbcc969b0659941493b3795c247e1360ccabc8ab68ec2c35f239b1517b789225706b2ab
		local url="http://d.web2.qq.com/channel/get_cface2?lcid="..parts[2].."&guid="..parts[3].."&to="..parts[1].."&count=5&time=1&clientid="..clientid.."&psessionid="..psessionid
		local referer=OP_referer_g
		print('Fetching '..parts[3]..' from tuin '..parts[1]..'...')
		local img=OP_Get(url,referer)
		if (string.len(img)~=0) then
			print("HandleP2PImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[1]..'_'..parts[2]..'_'..parts[3]
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("HandleP2PImage: GET failed for "..parts[3])
		end
	elseif #parts==2 then
		-- http://d.web2.qq.com/channel/get_offpic2?file_path=%2F0fb2dca1-72bf-4fce-9da8-3a34f7b2d217&f_uin=1263314672&clientid=8624094&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000369a00001a56016e04008aaeb8196d0000000a406b51516353535846616d000000288d8af4a0349faef00e36d3eb0fe0d96bae908dcdd4c677f20ff03cccdcb37b631b2dc9668392ded6
		parts[2]=parts[2]:sub(1,-5)
		local url="http://d.web2.qq.com/channel/get_offpic2?file_path=%2F"..parts[2].."&f_uin="..parts[1].."&clientid="..clientid.."&psessionid="..psessionid
		local referer=OP_referer_g
		print('Fetching '..parts[2]..' from tuin '..parts[1]..'...')
		local img=OP_Get(url,referer)
		if (string.len(img)~=0) then
			print("HandleP2PImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[1]..'_'..parts[2]..'.jpg'
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("HandleP2PImage: GET failed for "..parts[3])
		end
	else
		print("HandleP2PImage: Invalid input count: "..#parts..' uri='..uri)
		
		for k3,v3 in ipairs(parts) do
			print(" #"..k3.."="..v3)
		end
	end
end

-- OP Web Server calls this to fetch Qun Images
function HandleQunImage(uri)
	print('HandleQunImage: '..uri)

	-- gid/tuin/tm/server/port/fileid/name
	if vfwebqq==nil then
		print("HandleQunImage: vfwebqq not set!")
		return
	end

	local parts=Split(uri,"/")
	if #parts==7 then
		-- http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid=1058984205&uin=4200642849&rip=124.115.12.192&rport=8000&fid=1799487874&pic=%7B4E3FF3AA-C100-E536-6CE3-6E53427DF5F6%7D.jpg&vfwebqq=1aa616dedcc8f8214e83b28704960dd2233cae8f3c800980eea50e01e93768e8b32af25cffe14342&t=1343641932
		local url="http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid="..parts[1].."&uin="..parts[2].."&rip="..parts[4].."&rport="..parts[5].."&fid="..parts[6].."&pic="..parts[7].."&vfwebqq="..vfwebqq2.."&t="..parts[3]
		local referer="http://webqq.qq.com/"
		print('Fetching '..parts[7]..' from qun '..parts[1]..'...')
		
		print('HandleQunImage: ChkPt1');
		local img=OP_Get(url,referer)
		
		print('HandleQunImage: ChkPt2');

		if (img==nil or string.len(img)==0) then
			print('Fetch failed, try again')
			img=OP_Get(url,referer)
		end
		
		if (img~=nil and string.len(img)~=0) then
		print('HandleQunImage: ChkPt3');
			print("HandleQunImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[7]
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		print('HandleQunImage: ChkPt4');
		else
			print("HandleQunImage: GET failed for "..parts[7])
		end
		print('HandleQunImage: ChkPt5');
	else
		print("HandleQunImage: Invalid input count: "..#parts..' uri='..uri)
		
		for k3,v3 in ipairs(parts) do
			print(" #"..k3.."="..v3)
		end
	end
end

-- Return true if message is not a duplicate
function CheckMessageID(j)
	local msgid=j.msg_id..'_'..j.msg_id2
	
	for k,v in ipairs(msgpktbuffer) do
		if v==msgid then
			return false
		end
	end
	
	if #msgpktbuffer >= 100 then
		table.remove(msgpktbuffer,1)
	end
	
	table.insert(msgpktbuffer,msgid)
	
	print('msgpktbuffer	pool size='..#msgpktbuffer)
	
	return true
end

-- Sending all kinds of IM messages
-- ctype: 0=contact 1=qun 2=discussion 3=session
function OP_SendMessage(str)
	-- tuin	ctype	aux	msg
	-- {"content":[["font",{"color":"008080","name":"螳倶ｽ・,"size":12,"style":[0,0,0]}],"... "],"from_uin":3132949066,"msg_id":1297,"msg_id2":404816,"msg_type":9,"reply_ip":176881869,"time":1343452203,"to_uin":431533706}
	print('Entered OP_SendMessage')
	local parts=Split(str,'\t')
	print('After Split')
	print('OP_SendMessage: parts [1]='..parts[1]..' [2]='..parts[2]..' [3]='..parts[3])
	
	local str2
	local head
	local addkey=false
	if parts[2]=='0' then
		-- Contact Message
		head='r={"to":'..parts[1]..',"face":0,'
	elseif parts[2]=='1' then
		-- Qun Message
		head='r={"group_uin":'..parts[1]..','
	elseif parts[2]=='2' then
		-- Discussion Message
		head='r={"did":"'..parts[1]..'",'
	elseif parts[2]=='3' then
		-- Session Message: parts[3]=id
		-- http://d.web2.qq.com/channel/send_sess_msg2
		-- r:{"to":3043560234,"group_sig":"b83900ad33607db576d106ffba5ad4e2b8b1cadd660d059efdde23781b3b0c858b5a0452acd28d421af7867d606e4593","face":0,"content":"[\"test2\",\"\\n縲先署遉ｺïｼ壽ｭ､逕ｨ謌ｷ豁｣蝨ｨ菴ｿ逕ｨQ+ Webïｼ喇ttp://webqq.qq.com/縲曾",[\"font\",{\"name\":\"螳倶ｽ貼",\"size\":\"10\",\"style\":[0,0,0],\"color\":\"000000\"}]]","msg_id":61830001,"service_type":0,"clientid":"2183284","psessionid":"8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e31313500000156000013e3016e04008aaeb8196d0000000a40746e61464a7232656a6d0000002860a64b417129b223d3ec0759208fd0ef4eb92081e98a37d787a344ba6b23beced3935ef1a52664e0"}
		-- clientid:2183284
		-- psessionid:8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e31313500000156000013e3016e04008aaeb8196d0000000a40746e61464a7232656a6d0000002860a64b417129b223d3ec0759208fd0ef4eb92081e98a37d787a344ba6b23beced3935ef1a52664e0
		head='r={"to":'..parts[1]..',"group_sig":"'..sminfovalid[parts[3]]..'","face":0,'
	end
	
	local index=4
	local s
	local fn
	str2='"content":"['
	while parts[index]~=nil do
		s=parts[index]
		if index>4 then str2=str2..',' end
		if s:sub(1,6)=='[face:' then
			str2=str2..'[\\"face\\",'..s:sub(7)..']'
		elseif s:sub(1,5)=='[img]' then
			if parts[2]=='1' then
				if gface_key==nil or os.time()-cfacekeytime>432 then
					-- GFace Key required to send qun image (one time)
					Channel_GetGFaceSig2()
				end
				
				fn=OP_UploadQunImage(s:sub(6))
				if fn=='' then
					gface_key=nil
					error('ERRMESGQun Image Upload Failed')
				else
					if addkey==false then
						addkey=true
						head=head..'"group_code":'..parts[3]..',"key":"'..gface_key..'","sig":"'..gface_sig..'",'
					end
					str2=str2..fn
				end
			elseif parts[2]=='0' then
				-- Contact Message
				str2=str2..OP_UploadP2PImage(s:sub(6),parts[1])
			else
				str2=str2..'\\"\\"'
			end
		else
			str2=str2..'\\"'..EncodeUTF8(s,true)..'\\"'
		end
		index=index+1
	end
	str2=str2..',[\\"font\\",'
	str2=str2..'{\\"name\\":\\"'..EncodeUTF8(OP_fontname)..'\\",\\"size\\":\\"'..OP_fontsize..'\\",\\"style\\":['..OP_fontbold..','..OP_fontitalic..','..OP_fontunderline..'],\\"color\\":\\"'..OP_fontcolor..'\\"}]]",'
	str2=head..str2..'"msg_id":'..math.random(1000000,9999999)..',"clientid":"'..clientid..'","psessionid":"'..psessionid..'"}'
	
	print('str2='..str2)
	
	local url
	if parts[2]=='0' then
		-- Contact Message
		url='http://d.web2.qq.com/channel/send_buddy_msg2'
	elseif parts[2]=='1' then
		-- Qun Message
		url='http://d.web2.qq.com/channel/send_qun_msg2'
	elseif parts[2]=='2' then
		-- Discussion Message
		url='http://d.web2.qq.com/channel/send_discu_msg2'
	elseif parts[2]=='3' then
		-- Session Message
		url='http://d.web2.qq.com/channel/send_sess_msg2'
	end
	
	local ret=OP_Post(url,OP_referer_d,str2)
	
	if ret:find('retcode')==nil or (ret:byte(ret:len())~=10 and ret:byte(ret:len())~=125) then
		print('ERRRESPOP_SendMessage: First time send failed, try again. ref='..ret:byte(ret:len()))
		print('Dump: '..ret)
		ret=OP_Post(url,OP_referer_d,str2)
	end
	
	print('Ret='..ret)
	local j=JSON:decode(ret)
	
	if j.retcode~=0 then
		OP_BadReply('OP_SendMessage')
	end
end

-- Upload Qun Image to server
function OP_UploadQunImage(pathname)
	print('OP_UploadQunImage: upload '..pathname)
	print('vfwebqq='..vfwebqq)
	local url='http://up.web2.qq.com/cgi-bin/cface_upload?time='..GetTS()
	-- local url='http://posttestserver.com/post.php?dump&dir=mimqq4c&time='..GetTS()
	local ret=OP_Post(url,OP_referer_g,'',-1,{from='control',f='EQQ.Model.ChatMsg.callbackSendPicGroup',vfwebqq=vfwebqq,fileid=1,custom_face='\t'..pathname})
	print('result='..ret)
	-- test6[img]D:\Miranda-IM\587D08324C50F0E9584ACA35CA4B13F1.JPG[/img]
	-- <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPicGroup({'ret':0,'msg':'587D08324C50F0E9584ACA35CA4B13F1.jPg'});</script></head><body></body>
	-- <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPicGroup({'ret':4,'msg':'587D08324C50F0E9584ACA35CA4B13F1.jPg -6102 upload cface failed'});</script></head><body></body>
	-- <head><script type="text/javascript">document.domain='qq.com';parent.({'ret':2,'msg':'get uin and skey error getcgiparams failed asdf'});</script></head><body></body>
	if ret:find("'ret':0,")==nil and ret:find("'ret':4,")==nil then
		print('CFace upload failed, try again')
		ret=OP_Post(url,OP_referer_g,'',-1,{from='control',f='EQQ.Model.ChatMsg.callbackSendPicGroup',vfwebqq=vfwebqq,custom_face='\t'..pathname})
		print('result='..ret)		
	end
	
	if ret:find("'ret':0,")~=nil or ret:find("'ret':4,")~=nil then
		local discard
		local pos
		local pos2
		local fn
		discard,pos=ret:find("'msg':'")
		pos2=ret:find("'",pos+1)
		
		fn=ret:sub(pos+1,pos2-1)
		
		discard,pos=fn:find(' ')
		if discard~=nil then fn=fn:sub(1,pos-1) end
		
		return '[\\"cface\\",\\"group\\",\\"'..fn..'\\"]'
	else
		return ''
	end
end

-- Upload P2P Image to offline server
function OP_UploadP2PImage(pathname,tuin)
	print('OP_UploadP2PImage: upload '..pathname)
	local url='http://weboffline.ftn.qq.com/ftn_access/upload_offline_pic?time='..GetTS()
	local ret=OP_Post(url,OP_referer_g,'',-1,{callback='parent.EQQ.Model.ChatMsg.callbackSendPic',locallangid='2052',clientversion='1409',uin=tostring(OP_uin),skey=skey,appid='1002101',peeruin=tostring(tuin),file='\t'..pathname})
	print('result='..ret)
	-- test6[img]D:\Miranda-IM\587D08324C50F0E9584ACA35CA4B13F1.JPG[/img]
	-- <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPic({"retcode":0, "result":"OK", "progress":100, "filesize":17299, "fileid":"1", "filename":"%7B5E10B9BC-A2BD-F848-9CB3-239DFFA745B5%7D[2].jpg", "filepath":"/279dd1f0-e108-4482-a194-c158175beacf"});</script></head><body></body>
	local discard
	local pos1
	local pos2
	discard,pos1=ret:find('{')
	discard,pos2=ret:find('}')
	local j=JSON:decode(ret:sub(pos1,pos2))
	-- {"retcode":0, "result":"OK", "progress":100, "filesize":17299, "fileid":"1", "filename":"%7B5E10B9BC-A2BD-F848-9CB3-239DFFA745B5%7D[2].jpg", "filepath":"/279dd1f0-e108-4482-a194-c158175beacf"}
	if j.retcode==0 then
		return '[\\"offpic\\",\\"'..j.filepath..'\\",\\"'..j.filename..'\\",'..j.filesize..']';
	else
		return ' ';
	end
end

function OP_SearchBasic(uin)
	local verycode=' '
	while verycode==' ' do
		print('OP_SearchBasic: Requesting verycode')
		local url='http://captcha.qq.com/getimage?aid=1003901&r='..math.random()
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
	
	if verycode~=nil then
		-- http://s.web2.qq.com/api/search_qq_by_uin2?tuin=431533706&verifysession=h00f92b799cdcec210d8486f9a5382118273268767d55ce8f23c653a695e003e96d4bc642bff8aeacab&code=yumn&vfwebqq=1a2a18bc4c309d9f92d30735e699000ce5a1376a3e732be825dd16530777d7462c95849e4ac9f782&t=1357113592675
		local verifysession=OP_GetCookie('verifysession')
		print('verifysession='..verifysession)
		
		local j=CallAPI('search_qq_by_uin2','tuin='..uin..'&verifysession='..verifysession..'&code='..verycode)
		-- {"retcode":0,"result":{"face":0,"birthday":{"month":12,"year":1982,"day":7},"occupation":"0","phone":"-","allow":1,"college":"-","constel":11,"blood":0,"stat":20,"homepage":"-","country":"","city":"","uiuin":"","personal":"U+6DB4模鳴竭擱,U+59A6U+7E6B飲羶衄隱U+72DFU+FE5D","nick":"\"^_^2","shengxiao":11,"email":"431533706@qq.com","token":"0483c62b554cf08dbfaf9c0b33d066c797ac9f6dec4e8579","province":"","account":431533706,"gender":"unknown","tuin":515446227,"mobile":""}}
		
		if j~=nil then
			print('Call OP_AddSearchResult with nick='..j.nick..' and email='..j.email)
			OP_AddSearchResult(0,j.account,j.nick,j.email,j.token)
		else
			print('OP_SearchBasic: j==nil')
		end
		
		verycode=' '
		
		while verycode==' ' do
			print('OP_SearchBasic: Requesting verycode')
			local url='http://captcha.qq.com/getimage?aid=1003901&r='..math.random()
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
		
		if (verycode~=nil) then
			local resp=OP_Get('http://cgi.web2.qq.com/keycgi/qqweb/group/search.do?pg=1&perpage=10&all='..uin..'&c1=0&c2=0&c3=0&st=0&vfcode='..verycode..'&type=1&vfwebqq='..vfwebqq,'http://cgi.web2.qq.com/proxy.html?v=20110412001&callback=1&id=1')
			-- {"result":[{"GC":"","GD":"","GE":2571213,"GF":"","TI":"Miranda IM","GA":"","BU":"","GB":"","DT":"","RQ":"","QQ":"","MD":"","TA":"","HF":"","UR":"","HD":"","HE":"","HB":"","HC":"","HA":"","LEVEL":1,"PD":"","TX":"","PA":"","PB":"","CL":"","GEX":1677179817,"PC":""}],"retcode":0,"responseHeader":{"CostTime":15,"Status":0,"TotalNum":1,"CurrentNum":1,"CurrentPage":1}}
			print('OP_SearchBasic: resp='..resp)
			j=JSON:decode(resp)
			if j==nil then
				print('OP_SearchBasic: group j==nil')
			else
				local o=j.result
				if #o>0 then
					o=o[1]
					OP_AddSearchResult(1,o.GE,o.TI,'',o.GEX)
				end
			end
		end
		
		OP_EndOfSearch()
	end
end

function OP_AddUser(str)
	-- uin nick fn(1/0) ln(token) em msg
	print('OP_AddUser: '..str)
	local parts=Split(str,'\t')
	if parts[3]:sub(0,0)=='0' then
		-- r=%7B%22account%22%3A431533706%2C%22myallow%22%3A1%2C%22groupid%22%3A0%2C%22msg%22%3A%22test%22%2C%22token%22%3A%228166867181f7f073f1a6e3f1388873b693881ca8b2e47ab5%22%2C%22vfwebqq%22%3A%229d71312ad476d8a41a5a7eccb12b7905a07b731bf4d3298582caed048e2540f77b98623ff4e8e76b%22%7D
		local j=CallAPIPost('add_need_verify2',{account=parts[1],myallow=1,groupid=0,msg=parts[6],token=parts[4]},1)
		if j==nil then
			print('OP_AddUser: j==nil')
		end
	else
		local verycode=' '
		
		while verycode==' ' do
			print('OP_SearchBasic: Requesting verycode')
			local url='http://captcha.qq.com/getimage?aid=1003903&r='..math.random()
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
		
		if (verycode~=nil) then
			-- r=%7B%22gcode%22%3A728970475%2C%22code%22%3A%22qawh%22%2C%22vfy%22%3A%22h00ba8e4ec4fdb1b2d147e2158ceeacbe265cc51bc396c4a130a4b9409db65aab87efbccce92ae47a1d%22%2C%22msg%22%3A%22test%22%2C%22vfwebqq%22%3A%2282fd4c65ffa61fa585237e96b523ef29f6568090b88e92c46d14f3158eabcf0350c7b5883856538b%22%7D
			local verifysession=OP_GetCookie('verifysession')
			local j=CallAPIPost('apply_join_group2',{gcode=parts[4],code=verycode,vfy=verifysession,msg=parts[6]},1)
			if j==nil then
				print('OP_AddUser(Group): j==nil')
			end
		end
	end
end

function OP_Authorize(str)
	-- uin nick fn(1/0) ln em
	print('OP_Authorize: '..str)
	local parts=Split(str,'\t')
	-- r=%7B%22account%22%3A431533706%2C%22gid%22%3A0%2C%22mname%22%3A%22%22%2C%22vfwebqq%22%3A%229d71312ad476d8a41a5a7eccb12b7905a07b731bf4d3298582caed048e2540f77b98623ff4e8e76b%22%7D
	if parts[3]:sub(0,0)=='0' then
		local j=CallAPIPost('allow_and_add2',{account=parts[1],gid=0,mname=str[1]},1,1)
		if j.retcode~=100000 and j.retcode~=0 then
			print('OP_Authorize: j.retcode='..j.retcode)
		else
			print('OP_Authorize: response ok')
		end
	else
	end
end

function OP_Deny(str)
	-- uin nick fn ln em msg
	print('OP_Deny: '..str)
	local parts=Split(str,'\t')
	-- ?
	local j=CallAPIPost('deny_add_request2',{account=parts[1],msg=parts[6]},1,1)
	if j.result~=100000 and j.result~=0 then
		print('OP_Deny: j.result='..j.result)
	else
		print('OP_Deny: response ok')
	end
end

function OP_DeleteUser(str)
	-- uin
	local j=CallAPIPost('delete_friend','tuin='..str..'&delType=1',1)
	if j~=nil then
		print('OP_DeleteUser success')
	else
		print('OP_DeleteUser failed')
	end
end

-- Get signature (Long Nick)
function API_GetSingleLongNick2(tuin)
	--long nick=signature
	local j=CallAPI('get_single_long_nick2','tuin='..tuin);
	-- {"retcode":0,"result":[{"uin":431533706,"lnick":"....................."}]}
	
	if j~=nil then
		for i,v in ipairs(j) do
			print(v.uin..' Long Nick='..v.lnick)
			OP_UpdateProfile(v.uin,{signature=v.lnick})
		end
	end
end

-- Get QQ Level
function API_GetQQLevel2(tuin)
	local j=CallAPI('get_qq_level2','tuin='..tuin)
	-- {"retcode":0,"result":{"level":19,"days":476,"hours":2502,"remainDays":4,"tuin":431533706}}
	
	if j~=nil then
		j.tuin=nil
		OP_UpdateProfile(tuin,j)
	end
end

-- Get Friend Info
function API_GetFriendInfo2(tuin)
	local j=CallAPI('get_friend_info2','tuin='..tuin..'&verifysession=&code=')
	-- {"retcode":0,"result":{"face":0,"birthday":{"month":0,"year":0,"day":0},"occupation":"0","phone":"-","allow":1,"college":"-","uin":431533706,"constel":0,"blood":0,"homepage":"-","stat":10,"vip_info":0,"country":"","city":"","personal":"...............,........................","nick":"^_^2","shengxiao":0,"email":"431533706@qq.com","client_type":41,"province":"","gender":"unknown","mobile":""}}
	
	if j~=nil then
		local bt=j.birthday
		local nick=j.nick
		
		j.uin=nil -- Don't override uin which is already written
		j.birthday=nil -- Birthday needs to be reformatted instead of an array
		j.nick=nil
		j.Nick=nick
		
		if bt.year~=0 then j.birthday=bt.year..'/'..bt.month..'/'..bt.day end
		OP_UpdateProfile(tuin,j)
	else
		print('get_friend_info2 failed!')
	end
end

-- Hashing function for get_user_friends2
-- v4 20130726
if QQ_guf2hash==nil then
	QQ_guf2hash = function (b, i)
		local a={}
		local s=1
		while s<=b:len() do
			a[s]=tonumber(b:sub(s,s))
			s=s+1
		end
		
		local j=0
		local d=-1
		s=1
		while s<=#a do
			j=j+a[s]
			j=j%i:len()
			
			local c=0
			if j+4>i:len() then
				local l=4+j+i:len()
				local x=0
				while x<4 do
					if x<l then
						c=bit32.bor(c,bit32.lshift(bit32.band(i:byte(j+x+1),255),(3-x)*8))
					else
						c=bit32.bor(c,bit32.lshift(bit32.band(i:byte(x-l+1),255),(3-x)*8))
					end
					
					x=x+1
				end
			else
				local x=0
				while x<4 do
					c=bit32.bor(c,bit32.lshift(bit32.band(i:byte(j+x+1),255),(3-x)*8))
					x=x+1
				end
			end
			d=bit32.bxor(d,c)
			if d > 2147483647 then
				-- bit32.bxor() handles the number as QWORD!
				d = d - 4294967296;
			end
			
			s=s+1
		end
		
		a={}
		a[1]=bit32.band(bit32.rshift(d,24),255)
		a[2]=bit32.band(bit32.rshift(d,16),255)
		a[3]=bit32.band(bit32.rshift(d,8),255)
		a[4]=bit32.band(d,255)
		
		d={"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"}
		s=''
		j=1
		while j<=#a do
			s=s..d[bit32.band(bit32.rshift(a[j],4),15)+1]
			s=s..d[bit32.band(a[j],15)+1]
			
			j=j+1
		end
		
		return s
	end -- function P2
end

-- Get contact list
function API_GetUserFriends2()
	local j=CallAPIPost('get_user_friends2',{h='hello',hash=QQ_guf2hash(OP_uin,ptwebqq)})
	
	if j~=nil then
		local t={};
		local o;
		local uin;
		
		-- Info
		for k,v in ipairs(j.info) do
			t[tostring(v.uin)]={face=v.face,flag=v.flag,Nick=v.nick}
		end
		
		-- Group info
		for k,v in ipairs(j.friends) do
			o=t[tostring(v.uin)]
			if (o==nil) then
				print('API_GetUserFriends2: friends: table item for tuin '..v.uin..' not found!')
			else
				o.groupflag=v.flag
				o.group=v.categories
			end
		end
		
		-- Custom name
		for k,v in ipairs(j.marknames) do
			o=t[tostring(v.uin)]
			if (o==nil) then
				print('API_GetUserFriends2: marknames: table item for tuin '..v.uin..' not found!')
			else
				o.MyHandle=v.markname
			end
		end
		
		-- VIP info
		for k,v in ipairs(j.vipinfo) do
			o=t[tostring(v.u)]
			if (o==nil) then
				print('API_GetUserFriends2: vipinfo: table item for tuin '..v.uin..' not found!')
			else
				o.is_vip=v.is_vip
				o.vip_level=v.vip_level
			end
		end
		
		-- Groups
		for k,v in ipairs(j.categories) do
			print('API_GetUserFriends2: group - index='..v.index..' name='..v.name)
			OP_Group(v.index,v.name)
		end
		
		for k,v in pairs(t) do
			print('API_GetUserFriends2: contact - tuin='..k..' name='..v.Nick)
			OP_UpdateProfile(tonumber(k),v)
		end
	else
		OP_BadReply('get_user_friends2')
	end
end

-- Get Qun list
function API_GetGroupNameListMask2()
	local j=CallAPIPost('get_group_name_list_mask2',{})
	
	if j~=nil then
		-- {"retcode":0,"result":{"gmasklist":[],"gnamelist":[{"flag":17826833,"name":"........................","gid":3349286974,"code":1821691963},{"flag":17826817,"name":"Miranda IM","gid":3797684844,"code":2988863563},{"flag":17825793,"name":".........","gid":2943425861,"code":1629196628},{"flag":51380241,"name":".....................","gid":1083450505,"code":3707409410},{"flag":1064961,"name":".........2","gid":1847072296,"code":3588081341}],"gmarklist":[{"uin":1847072296,"markname":"Notice3_Group"}]}}
		local t={}
		
		-- Info
		for k,v in ipairs(j.gnamelist) do
			t[tostring(v.gid)]={IsQun=1,flag=v.flag,Nick='[G]'..v.name,code=v.code}
		end
		
		-- MyHandle
		for k,v in ipairs(j.gmarklist) do
			t[tostring(v.uin)].MyHandle=v.markname
		end
		
		for k,v in pairs(t) do
			OP_UpdateProfile(tonumber(k),v)
		end
	else
		OP_BadReply('get_user_friends2')
	end
end

-- Get online contacts
-- * Offline contacts not returned
function Channel_GetOnlineBuddies2()
	local j=CallChannel('get_online_buddies2','')
	
	if j~=nil then
		-- {"retcode":0,"result":[{"uin":1414495278,"status":"online","client_type":1},{"uin":907563397,"status":"online","client_type":1}]}
		for k,v in ipairs(j) do
			OP_ContactStatus(v.uin,StatusText2MIM(v.status),v.client_type)
			API_GetSingleLongNick2(v.uin)
		end
	else
		OP_BadReply('get_online_buddies2')
	end
end

-- Get list of discussion groups
function API_GetDiscusList()
	local j=CallAPI('get_discus_list','clientid='..clientid..'&psessionid='..psessionid)
	
	if j~=nil then
		-- {"retcode":0,"result":{"dnamelist":[{"did":3386761535,"name":"test"}],"dmasklist":[{"did":1000,"mask":0}]}}
		for k,v in ipairs(j.dnamelist) do
			print ('Init discussion list '..v.did..' ('..v.name..')')
			
			OP_AddTempContact(v.did,40072)
			OP_UpdateProfile(v.did,{IsDiscu=1,Nick='[D]'..v.name})
		end
	else
		OP_BadReply('get_discu_list_new2')
	end
end

-- Get list of discussion groups
function Channel_GetDiscuListNew2()
	local j=CallChannel('get_discu_list_new2','')
	
	if j~=nil then
		-- {"retcode":0,"result":{"dnamelist":[{"did":3386761535,"name":"test"}],"dmasklist":[{"did":1000,"mask":0}]}}
		for k,v in ipairs(j.dnamelist) do
			print ('Init discussion list '..v.did..' ('..v.name..')')
			
			OP_AddTempContact(v.did,40072)
			OP_UpdateProfile(v.did,{IsDiscu=1,Nick='[D]'..v.name})
		end
	else
		OP_BadReply('get_discu_list_new2')
	end
end

-- Retrieve GFace signature for sending qun image in messages
function Channel_GetGFaceSig2()
	local j=CallChannel('get_gface_sig2','')
	
	if j==nil or j.gface_key==nil or j.gface_sig==nil then
		print('GFace key retrieve error, try again')
		j=CallChannel('get_gface_sig2','')
	end
	
	if j~=nil then
		-- {"retcode":0,"result":{"reply":0,"gface_key":"Icn6VvY89nFcaGiH","gface_sig":"a2e96a31386ce2ebac8fe8426b1d0b80a462bbf1c7340bb40508e82b9cbe52c668d7b78b13a2f42fccb882836a7327c2dfc63573ba718279"}}
		print('GFace key acquired')
		gface_key=j.gface_key
		gface_sig=j.gface_sig
		cfacekeytime=os.time()
	else
		OP_BadReply('get_gface_sig2')
	end
end

-- Retrieve C2C signature for sending P2P image in messages
function Channel_GetC2CMsgSig2(gtuin,tuin)
	-- http://d.web2.qq.com/channel/get_c2cmsg_sig2?id=2002795244&to_uin=3043560234&service_type=0&clientid=2183284&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e31313500000156000013e3016e04008aaeb8196d0000000a40746e61464a7232656a6d0000002860a64b417129b223d3ec0759208fd0ef4eb92081e98a37d787a344ba6b23beced3935ef1a52664e0&t=1348136364186
	local j=CallChannel('get_c2cmsg_sig2','id='..gtuin..'&to_uin='..tuin..'&service_type=0&')
	
	if j~=nil then
		-- {"retcode":0,"result":{"type":0,"value":"b83900ad33607db576d106ffba5ad4e2b8b1cadd660d059efdde23781b3b0c858b5a0452acd28d421af7867d606e4593","flags":{"text":1,"pic":1,"file":1,"audio":1,"video":1}}}
		print('C2CMsg key acquired')
		return j.value
	else
		OP_BadReply('get_c2cmsg_sig2')
	end
	return nil;
end

-- Retrieve face image
-- * sometimes may fail because fh==nil?
function FACE_GetFace(tuin,tp,fid,uin)
	-- tp=1 fid=0
	local svr=10
	if tuin~=OP_uin then svr=tuin%10+1 end
	if uin==nil then uin=tuin end
	-- if tp==4 then svr=1 end
	
	local url='http://face'..svr..'.qun.qq.com/cgi/svr/face/getface?cache=0&type='..tp..'&fid='..fid..'&uin='..tuin..'&vfwebqq='..vfwebqq..'&t='..GetTS()
	print ('FACE_GetFace(): url='..url)
	local resp=OP_Get(url,OP_referer_s)
	local fn=OP_avatardir..'/'..uin..'.jpg'
	local fh=io.open(fn,"wb")
	if fh==nil then
		print('FACE_GetFace(): Failed opening '..fn..' for writing!')
	else
		fh:write(resp)
		fh:close()
	end
	
	OP_UpdateAvatar(tuin,fn)
end

-- Wrapper for FACE_GetFace
function OP_GetAvatar(str)
	-- isqun tuin uin code
	local parts=Split(str,'\t')
	-- local url
	
	if parts[1]=='1' then
		-- Qun
		-- url = 'http://face1.qun.qq.com/cgi/svr/face/getface?cache=0&type=4&fid=0&uin='..tuin..'&vfwebqq='..vfwebqq..'&t='..GetTS()
		FACE_GetFace(tonumber(parts[4]),4,0,tonumber(parts[3]))
	else
		-- String urlStr = "http://face10.qun.qq.com/cgi/svr/face/getface?cache=1&type=1&fid=0&uin=" + Auth.getMember().getAccount() + "&vfwebqq=" + Auth.getVfwebqq() + "&t=" + System.currentTimeMillis();
		FACE_GetFace(tonumber(parts[2]),1,0,tonumber(parts[3]))
	end
end

-- Main loop
-- * This is the only Channel function that uses POST, so no common fetcher
function OP_PollLoop()
	local pollurl='http://d.web2.qq.com/channel/poll2'
	local polldata='r='..JSON:encode({clientid=clientid,psessionid=psessionid,key=0,ids={}})..'&clientid='..clientid..'&psessionid='..psessionid
	local resp
	
	
	while true do
		print('OP_PollLoop: Polling for result...')
		resp=OP_Post(pollurl,OP_referer_d,polldata)
		print('Poll result='..resp)
		
		if resp:byte(1)==123 and resp:byte(resp:len())~=125 then
			local j=JSON:decode(resp)
			local o

			if j.retcode==0 then
				for k,v in ipairs(j.result) do
					if v.poll_type=='group_message' then
						if v.value.msg_type==43 then
							o=v.value
							if CheckMessageID(o)==true then
								local gtuin=o.from_uin
								local gext=o.group_code
								local tuin=o.send_uin
								local tp=o.msg_type -- 43
								local tm=o.time
								local msg=ProcessMessage(o.content,tuin,gext,o.msg_id,tm)
								print('GM '..gtuin..'['..tuin..']: '..msg)
								if quninfovalid[tostring(gtuin)]==nil then
									OP_GetGroupInfoExt2(gext)
								end
								OP_GroupMessage(gtuin,tuin,tm,msg)
							else
								print('Skipped duplicated message '..o.msg_id..'_'..o.msg_id2)
							end
						else
							print('Skipped group_message with unknown msg_type '..v.value.msg_type)
							print(JSON:encode(v.value))
						end -- msg_type
					elseif v.poll_type=='discu_message' then
						-- {"retcode":0,"result":[{"poll_type":"discu_message","value":{"msg_id":27596,"from_uin":10000,"to_uin":431533706,"msg_id2":36247,"msg_type":42,"reply_ip":176752214,"did":3386761535,"send_uin":165649255,"seq":1,"time":1347959064,"info_seq":1,"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],"test "]}}]}
						if v.value.msg_type==42 then
							o=v.value
							if CheckMessageID(o)==true then
								local did=o.did
								local tuin=o.send_uin
								local tp=o.msg_type -- 42
								local tm=o.time
								local msg=ProcessMessage(o.content,tuin,did,o.msg_id,tm)
								print('DM '..did..'['..tuin..']: '..msg)
								if quninfovalid[tostring(did)]==nil then
									OP_AddTempContact(did,40072) -- 40072=ID_STATUS_ONLINE
									OP_GetDiscuInfo(did)
								end
								OP_GroupMessage(did,tuin,tm,msg)
							else
								print('Skipped duplicated message '..o.msg_id..'_'..o.msg_id2)
							end
						else
							print('Skipped discu_message with unknown msg_type '..v.value.msg_type)
							print(JSON:encode(v.value))
						end -- msg_type
					elseif v.poll_type=='message' then
						-- {"content":[["font",{"color":"008080","name":"螳倶ｽ・,"size":12,"style":[0,0,0]}],"... "],"from_uin":3132949066,"msg_id":1297,"msg_id2":404816,"msg_type":9,"reply_ip":176881869,"time":1343452203,"to_uin":431533706}
						-- {"retcode":0,"result":[{"poll_type":"message","value":{"msg_id":18631,"from_uin":3291940000,"to_uin":431533706,"msg_id2":902526,"msg_type":9,"reply_ip":176498347,"time":1347687817,"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],["cface","5E10B9BCA2BDF8489CB3239DFFA745B5.jpg",""]," "]}}]}
-- http://d.web2.qq.com/channel/get_cface2?lcid=18631&guid=5E10B9BCA2BDF8489CB3239DFFA745B5.jpg&to=3291940000&count=5&time=1&clientid=42759885&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000049500001266016e04008aaeb8196d0000000a40504c4847507a4266466d00000028f2ed4db4ddbcc969b0659941493b3795c247e1360ccabc8ab68ec2c35f239b1517b789225706b2ab
-- Referer:http://webqq.qq.com/

						if (v.value.msg_type==9) then
							o=v.value
							if CheckMessageID(o)==true then
								local tuin=o.from_uin
								local tm=o.time
								local msg=ProcessMessage(o.content,tuin,0,o.msg_id,tm)
								print('CM '..tuin..': '..msg)
								OP_ContactMessage(tuin,tm,msg)
							else
								print('Skipped duplicated message '..o.msg_id..'_'..o.msg_id2)
							end
						else
							print('Skipped message with unknown msg_type '..v.value.msg_type)
							print(JSON:encode(v.value))
						end
					elseif v.poll_type=='buddies_status_change' then
						-- {"uin":3132949066,"status":"online","client_type":1}
						print('Buddy '..v.value.uin..' changed status to '..v.value.status)
						OP_ContactStatus(v.value.uin,StatusText2MIM(v.value.status),v.value.client_type)
						if v.value.status~='offline' then API_GetSingleLongNick2(v.value.uin) end
					elseif v.poll_type=='kick_message' then
						error('ERRDUPL')
					elseif v.poll_type=='input_notify' then
						-- {"poll_type":"input_notify","value":{"msg_id":42953,"from_uin":165649255,"to_uin":431533706,"msg_id2":1813669024,"msg_type":121,"reply_ip":4294967295}}
						print('Buddy '..v.value.from_uin..' is entering text...')
						OP_TypingNotify(v.value.from_uin)
					elseif v.poll_type=='group_web_message' then
						o=v.value
						if CheckMessageID(o)==true then
							local gtuin=o.from_uin
							local tuin=o.send_uin
							local xml=o.xml
							print('GM '..gtuin..'['..tuin..']: '..xml)
							
--GM 3130608083[0]: <?xml version="1.0" encoding="utf-8"?><d><n t="h" u="85379868"
-- i="201" s="s.qun.qq.com/god/images/channelicon.png" c="75797A"/><n t="t" s="驍
--隸ｷ謔ｨ荳襍ｷ隗ら恚"/><n t="t" s="縲頑ｴｻ蜉帛ｽｱ髯｢:螟ｧ蝮怜､ｴ譛牙､ｧ譎ｺ諷ｧ縲・/><n t
--="b"/><n t="t" s="00:11:46"/><n t="r"/><n t="i" s="s.qun.qq.com/god/images/chann
--el2084914015.jpg" l="__/air/qqlive"/><n t="r"/><n t="b"/><n t="t" l="__/air/qqli
--ve" s="隗ら恚謖・ﾍ"/></d>							

							local fstart
							local fend
							local fend2
							local uin
							local t

							fstart,fend=xml:find('t="h"')

							if fstart~=nil then
								fstart,fend=xml:find('u="',fend)
								fstart,fend2=xml:find('"',fend+1)
								uin=xml:sub(fend+1,fend2-1)
								t=uin
								
								fstart,fend=xml:find('t="t" s="',fend2)
								while fstart~=nil do
									fstart,fend2=xml:find('"/>',fend+1)
									print('fstart='..fstart..' fend2='..fend2)
									t=t..xml:sub(fend+1,fstart-1)
									fstart,fend=xml:find('t="t" s="',fend2)
								end
								
								print('t='..t)
								OP_GroupMessage(gtuin,tuin,0,t)
							end

						else
							print('Skipped duplicated web message '..o.msg_id..'_'..o.msg_id2)
						end
					elseif v.poll_type=='sess_message' then
						-- {"retcode":0,"result":[{"poll_type":"sess_message","value":{"msg_id":7522,"from_uin":3043560234,"to_uin":431533706,"msg_id2":369799,"msg_type":140,"reply_ip":176882136,"time":1348136233,"id":2002795244,"ruin":85379868,"service_type":0,"flags":{"text":1,"pic":1,"file":1,"audio":1,"video":1},"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],"test "]}}]}
						-- {"retcode":0,"result":[{"poll_type":"sess_message","value":{"msg_id":7523,"from_uin":3043560234,"to_uin":431533706,"msg_id2":913871,"msg_type":140,"reply_ip":176882159,"time":1348136550,"id":2002795244,"ruin":85379868,"service_type":0,"flags":{"text":1,"pic":1,"file":1,"audio":1,"video":1},"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],["pic_id","5E10B9BC-A2BD-F848-9CB3-239DFFA745B5"],["image","jpg",0]," "]}}]}
						
						if (v.value.msg_type==140) then
							o=v.value
							if CheckMessageID(o)==true then
								-- Note: gtuin and tuin can both already exists, so UIN needs to be unique and independent of gtuin and tuin
								-- sminfovaild stores sig2
								local gtuin=o.id
								local tuin=o.from_uin
								local tm=o.time
								local msg=ProcessMessage(o.content,tuin,0,o.msg_id,tm)
								print('SM '..gtuin..'['..tuin..']: '..msg)
								if sminfovalid[gtuin..'_'..tuin]==nil then
									API_GetStrangerInfo(gtuin,tuin)
								end
								OP_SessionMessage(gtuin,tuin,tm,msg)
							else
								print('Skipped duplicated session message '..o.msg_id..'_'..o.msg_id2)
							end
						else
							print('Skipped session message with unknown msg_type '..v.value.msg_type)
							print(JSON:encode(v.value))
						end
					elseif v.poll_type=='system_message' then
						-- [{"poll_type":"system_message","value":{"account":85379868,"client_type":41,"from_uin":3441364886,"msg":"test","seq":31805,"stat":10,"type":"verify_required","uiuin":""}}]
						o=v.value
						if o.type=='verify_required' then
							-- isqun account nick fn ln email msg
							OP_RequestJoin(0,o.account,'','','',o.from_uin,o.msg)
						else
							print('system_message: '..JSON:encode(o))
						end
					elseif v.poll_type=='buddylist_change' then
						-- [{"poll_type":"buddylist_change","value":{"added_friends":[{"groupid":0,"uin":1754314370}],"removed_friends":[]}}]
						o=v.value
						for k,v in ipairs(o.added_friends) do
							API_GetFriendInfo2(v.uin)
						end
						for k,v in ipairs(o.removed_friends) do
							-- TODO
						end
					else
						print(JSON:encode(j.result))
					end
				end
				-- [{"poll_type":"sys_g_msg","value":{"from_uin":3792974046,"gcode":2273134460,"msg_id":28386,"msg_id2":822524,"msg_type":34,"old_member":1754314370,"op_type":2,"reply_ip":176882264,"t_gcode":58914413,"t_old_member":"","to_uin":431533706,"type":"group_leave"}}]
			elseif j.retcode==116 then
				-- Update ptwebqq
				ptwebqq=j.p
				cookie="pgv_pvid="..pgv_pvid.."; pgv_info=pgvReferrer=&ssid="..ssid.."; ptui_loginuin="..g_uin.."; ptwebqq="..ptwebqq
				OP_SetCookie('ptwebqq='..ptwebqq..'; Path=/; Domain=.qq.com')
				print('Poll2: ptwebqq updated')
			elseif j.retcode==110 or j.retcode==100 then
				error('ERRLOFF')
			elseif j.retcode==121 or j.retcode==120 then
				error('ERRDUPL')
			-- elseif j.retcode==102 then
			-- 	print('Poll2: Poll completed, restart')
			elseif j.retcode~=102 then
				error('ERRPOLL'..j.retcode..':'..j.errmsg)
			end
		else
			print('Possibly server error, try again: ')
		end
	end
end

-- Get Real UIN for tuin
-- * Will user get blocked when calling too frequently?
function OP_GetRealUIN(gcode)
	ruin_result=0
	
	local j=CallAPI('get_friend_uin2','tuin='..gcode..'&verifysession=&type=1&code=');
	
	if j~=nil then
		ruin_result=j.account
		return j.account
	end
	
	return 0
end

-- Get External ID for quin tuin
-- * Will user get blocked when calling too frequently?
function OP_GetRealExternalID(tuin)
	local j=CallAPI('get_friend_uin2','tuin='..tuin..'&verifysession=&type=4&code=');
	
	if j~=nil then
		return j.account
	end
	
	return 0
end

-- Change user status
-- status is Miranda IM status (400xx)
function OP_ChangeStatus(status)
	local statustext=StatusMIM2Text(status)
	local j=CallChannel('change_status2','newstatus='..statustext..'&')
	
	print('Change status to '..statustext);
	
	if j==nil then
		OP_BadReply('change_status2')
	end
end

-- Get Qun Information by group code (! not tuin!)
function OP_GetGroupInfoExt2(gcode)
	local j=CallAPI('get_group_info_ext2','gcode='..gcode)
	
	-- {"retcode":0,"result":{"stats":[{"client_type":1,"uin":1649606722,"stat":10},{"client_type":41,"uin":431533706,"stat":10}],"minfo":[{"nick":"j85379868","province":"福建","gender":"male","uin":1355897758,"country":"中国","city":""},{"nick":"^_^","province":"","gender":"male","uin":1649606722,"country":"","city":""},{"nick":"^_^2","province":"","gender":"unknown","uin":431533706,"country":"","city":""}],"ginfo":{"face":0,"memo":"","class":10048,"uin":2138914413,"fingermemo":"","code":4101376875,"createtime":1206845216,"flag":1064961,"level":0,"name":"測試群2","gid":2523811027,"owner":431533706,"markname":"Notice3_Group","members":[{"muin":1355897758,"mflag":0},{"muin":1649606722,"mflag":73},{"muin":431533706,"mflag":0}],"option":2},"cards":[{"muin":1649606722,"card":"^_^1中文"}],"vipinfo":[{"vip_level":0,"u":1355897758,"is_vip":0},{"vip_level":0,"u":1649606722,"is_vip":0},{"vip_level":0,"u":431533706,"is_vip":0}]}}
	
	if j~=nil then
		local t={}
		
		-- Info
		for k,v in ipairs(j.minfo) do
			t[tostring(v.uin)]=v.nick
		end
		
		-- MyHandle
		if j.cards~=nil then
			for k,v in ipairs(j.cards) do
				t[tostring(v.muin)]=v.card
			end
		end
		
		-- "ginfo":{"face":0,"memo":"","class":10048,"uin":2138914413,"fingermemo":"","code":4101376875,"createtime":1206845216,"flag":1064961,"level":0,"name":"測試群2","gid":2523811027,"owner":431533706,"markname":"Notice3_Group","members":[{"muin":1355897758,"mflag":0},{"muin":1649606722,"mflag":73},{"muin":431533706,"mflag":0}],"option":2}
		
		local ginfo=j.ginfo
		local tuin=ginfo.gid
		-- ginfo.tuin=tuin
		ginfo.gid=nil
		local nick='[G]'..ginfo.name
		ginfo.Nick=nick
		ginfo.name=nil
		local signature=ginfo.memo
		ginfo.signature=signature
		ginfo.memo=nil
		if ginfo.markname~=nil and ginfo.markname~='' then
			local markname=ginfo.markname
			ginfo.MyHandle=markname
			ginfo.markname=nil
		end
		print('tuin='..tuin..' code='..ginfo.code)
		OP_UpdateQunInfo(tuin,ginfo)
		OP_UpdateQunMembers(tuin,t)
		
		quninfovalid[tostring(tuin)]=1
	else
		OP_BadReply('get_group_info_ext2')
	end
end

-- Get discussion group information
-- did is just another tuin-like thing
-- * Is did fixed between logins?
function OP_GetDiscuInfo(did)
	-- http://d.web2.qq.com/channel/get_discu_info?did=3386761535&clientid=95905555&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e31313500002f9700001356016e04008aaeb8196d0000000a40625050594e58596d566d000000283e90b81d508049c73f030e1c08148075c9dbba1ed50e5819ad33b94f536327b4c6908583ba043ebd&vfwebqq=3e90b81d508049c73f030e1c08148075c9dbba1ed50e5819ad33b94f536327b4c6908583ba043ebd&t=1347959147113
	local j=CallAPI('get_discu_info','did='..did..'&clientid='..clientid..'&psessionid='..psessionid)
	
	-- {"retcode":0,"result":{"info":{"did":3386761535,"discu_owner":165649255,"discu_name":"test","info_seq":2,"mem_list":[{"mem_uin":165649255,"ruin":85379868},{"mem_uin":1940952503,"ruin":431533686},{"mem_uin":431533706,"ruin":431533706}]},"mem_status":[{"uin":165649255,"status":"online","client_type":1},{"uin":1940952503,"status":"away","client_type":1},{"uin":431533706,"status":"online","client_type":41}],"mem_info":[{"uin":165649255,"nick":"j85379868"},{"uin":1940952503,"nick":"^_^"},{"uin":431533706,"nick":"^_^2"}]}}
	
	if j~=nil then
		local t={}
		
		-- Info
		for k,v in ipairs(j.mem_info) do
			t[tostring(v.uin)]='[D]'..v.nick
		end
		
		-- "info":{"did":3386761535,"discu_owner":165649255,"discu_name":"test","info_seq":2,"mem_list":[{"mem_uin":165649255,"ruin":85379868},{"mem_uin":1940952503,"ruin":431533686},{"mem_uin":431533706,"ruin":431533706}]}
		local ginfo=j.info
		
		local tuin=ginfo.did
		ginfo.gid=nil
		ginfo.IsDiscu=1
		local nick=ginfo.discu_name
		ginfo.Nick=nick
		ginfo.name=nil
		print('did='..tuin)
		OP_UpdateQunInfo(tuin,ginfo)
		OP_UpdateQunMembers(tuin,t)
		
		quninfovalid[tostring(tuin)]=1
	else
		OP_BadReply('get_discu_info')
	end
end

-- Get contact info in Session Message
function API_GetStrangerInfo(gtuin,tuin)
	-- http://s.web2.qq.com/api/get_stranger_info2?tuin=3043560234&verifysession=&gid=0&code=&vfwebqq=60a64b417129b223d3ec0759208fd0ef4eb92081e98a37d787a344ba6b23beced3935ef1a52664e0&t=1348136241041
	local j=CallAPI('get_stranger_info2','tuin='..tuin..'&verifysession=&gid=0&code=')
	
	-- {"retcode":0,"result":{"face":0,"birthday":{"month":0,"year":0,"day":0},"phone":"","occupation":"","allow":1,"college":"","uin":3043560234,"blood":0,"constel":0,"homepage":"","stat":10,"country":"荳ｭ蝗ｽ","city":"","personal":"","nick":"j85379868","shengxiao":0,"email":"","token":"5c206e43d03e5f6a52cac1d16351db53a33ce7af423b151f","client_type":1,"province":"遖丞ｻｺ","gender":"male","mobile":"-"}}
	
	if j~=nil then
		local bt=j.birthday
		local nick=j.nick
		
		smindex=smindex+1
		
		j.uin=smindex
		j.tuin=smindex
		j.sm_gtuin=gtuin
		j.sm_tuin=tuin
		j.sm_id=gtuin..'_'..tuin
		j.birthday=nil
		j.nick=nil
		j.Nick=nick
		j.IsSession=1
		if bt.year~=0 then j.birthday=bt.year..'/'..bt.month..'/'..bt.day end
		sminfovalid[j.sm_id]=Channel_GetC2CMsgSig2(gtuin,tuin)
		OP_AddTempContact(smindex,40072)
		OP_UpdateProfile(smindex,j)
	else
		print('get_stranger_info2 failed!')
	end
end

function test()
	print('Invoking test function')
	OP_UploadQunImage('D:\\Miranda-IM\\587D08324C50F0E9584ACA35CA4B13F1.JPG')
end


-- W+ does not support image in session message, this is what it looks like
--{"retcode":0,"result":[{"poll_type":"sess_message","value":{"msg_id":7523,"from_uin":3043560234,"to_uin":431533706,"msg_id2":913871,"msg_type":140,"reply_ip":176882159,"time":1348136550,"id":2002795244,"ruin":85379868,"service_type":0,"flags":{"text":1,"pic":1,"file":1,"audio":1,"video":1},"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],["pic_id","5E10B9BC-A2BD-F848-9CB3-239DFFA745B5"],["image","jpg",0]," "]}}]}


-- Temp
local send = function (url, params)
	-- This function is non-webqq
	
	local s
	local r
	
	if url:find("s.web2")~=nil then
		r=referer_s
	elseif url:find("d.web2")~=nil then
		r=referer_d
	else
		r=referer_w
	end
	
	-- TODO: encode r all the time
	-- params.data is a table with several items when in GET mode
	local p='';
	for k,v in pairs(params.data) do
		if p:len()>0 then p=p..'&' end
		p=p..k..'='..v
	end
	
	for i=1,2 do
		if params.method~=nil and params.method=='POST' then
			s=OP_Post(url,r,p)
		else
			s=OP_Get(url..'?'..p,r)
		end
		
		if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
			-- not 125, should be 10
			if i==1 then
				print('eqq.send: First time data failed for '..url..', try again') -- . ref='..s:byte(s:len()))
			else
				print('eqq.send: Second time data failed')
				params.onError()
				return
			end
		else
			break
		end
	end -- for i
	
	local j=JSON:decode(s)
	if j==nil then
		print('eqq.send: JSON decode failed')
		params.onError()
	else
		params.onSuccess(j,s)
	end
end -- func send

local cgi_module = function (d, e)
	local proxysend = function (e, c)
		local p={
			method=c.method,
			data={
				r=JSON:encode(c.param)
			},
			onSuccess=c.onSuccess,
			onError=c.onError
		}
		if p.method==nil then p.method="GET" end
		
		send(e,p)
	end -- inner func proxysend

	-- d=url e=params
	e.onError = e.errback
	if e.onError==nil then e.onError=function() end end
	if e.onTimeout==nil then e.onTimeout=function() end end
	e.onSuccess = function (d)
		e.callback(d, c)
	end
	proxysend(d, e)
end -- func cgi_module

local cgi_module_d = function (d, e)
	e.onError = e.errback
	if e.onError==nil then e.onError=function () end end
	e.onTimeout = e.timeback
	if e.onTimeout==nil then e.onTimeout=function () end end
	local c = function () end
	e.onSuccess = function (d)
		e.callback(d, c)
	end
	send(d, e)
end

local start=function()
	OP_uin=g_uin
	vfwebqq2=vfwebqq
	
	print('Performing post login actions...')
	if (OP_prestop==false) then API_GetSingleLongNick2(OP_uin) end
	if (OP_prestop==false) then API_GetQQLevel2(OP_uin) end
	if (OP_prestop==false) then API_GetFriendInfo2(OP_uin) end
	if (OP_prestop==false) then FACE_GetFace(OP_uin,1,0) end
	if (OP_prestop==false) then API_GetUserFriends2() end
	if (OP_prestop==false) then Channel_GetOnlineBuddies2() end
	if (OP_prestop==false) then API_GetGroupNameListMask2() end
	-- if (OP_prestop==false) then Channel_GetDiscuListNew2() end
	if (OP_prestop==false) then API_GetDiscusList() end
	
	if (OP_prestop==false) then
		OP_resume=true
		OP_PollLoop()
		-- local fn=coroutine.create(OP_PollLoop)
		-- coroutine.resume(fn)
		print('End of loop')
	else
		error('ERRLOG2')
	end
end -- start

local init2=function()
	-- -> login -> sendLogin
	local sendLogin=function(c)
		c.clientid = clientid;
		c.psessionid = psessionid;
		
		print('eqq.init2: clientid='..clientid)
		
		send(m.."channel/login2", {
				method="POST",
				data={
						r=JSON:encode(c)
				},
				onSuccess=function(a,c) -- sendLoginSuccess
					if a.retcode==0 then
						local b=a.result
						vfwebqq=b.vfwebqq
						psessionid=b.psessionid
						clientkey=b.clientkey
						
						OP_CreateThreads();
						OP_LoginSuccess()
						start()
					elseif a.retcode==103 then
						error('ERRLOG2你的登录已失效，请重新登录。')
					elseif a.retcode==106 then
						error('ERRLOG2Overload(UinNotInWhitelist)')
					elseif a.retcode==111 then
						error('ERRLOG2Overload(UinInBlacklist)')
					elseif a.retcode==112 then
						error('ERRLOG2Overload')
					elseif a.retcode==100000 or a.retcode==100001 or a.retcode==100002 then
						error('ERRLOG2验证信息过期，请重新登录！')
					end
				end,
				onError=function(a) -- sendLoginError
					error('ERRLOG2')
				end
		})
	end
	
	local dna_result_key=''
	psessionid=nil
	
	-- clientid = String(d.random(0, 99)) + String((new Date).getTime() % 1E6),
	clientid=math.random(10000000,99999999)
	ptwebqq=OP_GetCookie('ptwebqq')
	
	local b = {
		status=StatusMIM2Text(OP_status),
		ptwebqq=ptwebqq,
		passwd_sig=dna_result_key,
		clientid=clientid
	}
	
	sendLogin(b)
end

init2()
