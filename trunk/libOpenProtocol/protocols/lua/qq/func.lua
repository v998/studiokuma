print('func.lua for MIMQQ4c')
JSON=require('JSON')

-- Referers required for each tunnel
OP_referer_s='http://s.web2.qq.com/proxy.html?v=20110412001&callback=1&id=3'
OP_referer_d='http://d.web2.qq.com/proxy.html?v=20110331002&callback=1&id=2'
OP_referer_g='http://webqq.qq.com'

-- State management
quninfovalid={} -- Qun Information receive
sminfovalid={}  -- Session Message Information receive
msgpktbuffer={} -- Duplicated message checking
smindex=0       -- Session Message index (1 based)

-- Publish bad reply error
function OP_BadReply(str)
	error('ERRBRPY'..str)
end

-- Separate double-quoted strings
function CaptureQuotedParts(a)
	local ret={}
	local discard,pos
	local pos2=0
	local pos3
	local pass2=false
	
	while true do
		discard,pos=string.find(a,"'",pos2+1,true)
		if discard==nil then break end
		
		pos=pos+1
		
		pass2=false
		pos3=pos
		while pos3>0 do
			pos2=string.find(a,"'",pos3,true)
		
			if string.sub(a,pos2-1,pos2-1)=='\\' then
				print('CaptureQuotedParts: Escape sequence detected')
				pos3=pos2+1
			else
				pos3=0
			end
		end
		
		table.insert(ret,string.sub(a,pos,pos2-1))
		pos2=pos2+1
	end

	return ret
end

-- Get 10-characters timestamp
function GetTS()
	return os.time(os.date('*t'))..math.random(100,999)
end

-- Calling API functions with GET method
function CallAPI(func,extra)
	local url='http://s.web2.qq.com/api/'..func..'?'..extra..'&vfwebqq='..vfwebqq..'&t='..GetTS()
	local s=OP_Get(url,OP_referer_s)
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		-- not 125, should be 10
		print('CallAPI: First time data failed for '..func..', try again')
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Get(url,OP_referer_s)
	end
	
	local j=JSON:decode(s)
	
	if j~=nil and j.retcode==0 then
		return j.result
	else
		OP_BadReply(func)
		return nil
	end
end

-- Calling API functions with POST method
function CallAPIPost(func,j1)
	local url='http://s.web2.qq.com/api/'..func
	j1['vfwebqq']=vfwebqq
	local s=OP_Post(url,OP_referer_s,'r='..JSON:encode(j1))
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		print('CallAPIPost: First time data failed for '..func..', try again')
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Post(url,OP_referer_s,'r='..JSON:encode(j1))
	end
	
	local j=JSON:decode(s)
	
	if j~=nil and j.retcode==0 then
		return j.result
	else
		OP_BadReply(func)
		return nil
	end
end

-- Calling Channel function with GET method
function CallChannel(func,extra)
	local url='http://d.web2.qq.com/channel/'..func..'?'..extra..'clientid='..clientid..'&psessionid='..psessionid..'&t='..GetTS()
	local s=OP_Get(url,OP_referer_d)
	
	if s:find('retcode')==nil or (s:byte(s:len())~=10 and s:byte(s:len())~=125) then
		print('CallChannel: First time data failed for '..func..', try again')
		if s:find('retcode')~=nil then print('byte='..s:byte(s:len())) end
		s=OP_Get(url,OP_referer_d)
	end
	
	local j=JSON:decode(s)
	
	if j~=nil and j.retcode==0 then
		return j.result
	else
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
function ProcessMessage(msg,tuin,gid,msgid)
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
						s=s..'[img]http://127.0.0.1:'..OP_imgserverport..'/qunimages/'..OP_uin..'/'..gid..'/'..tuin..'/'..string.gsub(o2.server,':','/')..'/'..o2.file_id..'/'..o2.name..'[/img]'
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

-- LUA does not have a string split function, this does it
function Split(str, delim, maxNb)
	-- Eliminate bad cases...
	if string.find(str, delim) == nil then
		return { str }
	end
	if maxNb == nil or maxNb < 1 then
		maxNb = 0    -- No limit
	end
	local result = {}
	local pat = "(.-)" .. delim .. "()"
	local nb = 0
	local lastPos
	for part, pos in string.gmatch(str, pat) do
		nb = nb + 1
		result[nb] = part
		lastPos = pos
		if nb == maxNb then break end
	end
	-- Handle the last field
	if nb ~= maxNb then
		result[nb + 1] = string.sub(str, lastPos)
	end
	return result
end

-- OP Web Server calls this to fetch P2P Images
function HandleP2PImage(uri)
	-- tuin/msgid/name
	-- tuin/filename
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
	-- gid/tuin/server/port/fileid/name
	local parts=Split(uri,"/")
	if #parts==6 then
		-- http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid=1058984205&uin=4200642849&rip=124.115.12.192&rport=8000&fid=1799487874&pic=%7B4E3FF3AA-C100-E536-6CE3-6E53427DF5F6%7D.jpg&vfwebqq=1aa616dedcc8f8214e83b28704960dd2233cae8f3c800980eea50e01e93768e8b32af25cffe14342&t=1343641932
		local url="http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid="..parts[1].."&uin="..parts[2].."&rip="..parts[3].."&rport="..parts[4].."&fid="..parts[5].."&pic="..parts[6].."&vfwebqq="..vfwebqq.."&t="..os.time(os.date('*t'))
		local referer=OP_referer_g
		print('Fetching '..parts[6]..' from qun '..parts[1]..'...')
		
		local img=OP_Get(url,referer)
		if (string.len(img)==0) then
			print('Fetch failed, try again')
			img=OP_Get(url,referer)
		end
		
		if (string.len(img)~=0) then
			print("HandleQunImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[6]
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("HandleQunImage: GET failed for "..parts[6])
		end
	else
		print("HandleQunImage: Invalid input count: "..#parts..' uri='..uri)
		
		for k3,v3 in ipairs(parts) do
			print(" #"..k3.."="..v3)
		end
	end
end

-- Encodes UTF-8 encoded text to JS encoded
-- splitNL: true if encoded for IM messages, needs more escaping
function EncodeUTF8(str,splitNL)
	local buffer={0,0,0,0}
	local str2=''
	local mode=0
	local current=0
	local base
	local cp
	local mt
	local ch

	for k,v in ipairs(table.pack(string.byte(str,1,string.len(str)))) do
		if mode==0 then
			if v>=248 then
				print('ERROR: Bad parsing')
			elseif v==13 then
				-- skip CR
				mode=-1
			elseif v==10 then
				-- LF
				mode=-1
				if splitNL~=nil and splitNL==true then 
					str2=str2..'\\",\\"'
					str2=str2..'\\\\n'
				else
					str2=str2..'\\n'
				end
			elseif v==92 then
				mode=-1
				if splitNL~=nil and splitNL==true then 
					str2=str2..'\\\\\\\\'
				else
					str2=str2..'\\\\'
				end
			elseif v==34 then
				-- "
				mode=1
				str2=str2..'\\\\'
			elseif v==38 or v==59 or v==39 then
				-- &';
				mode=1
			else
				mt=8
				mode=4
				ch=240
				while ch>=128 do
					if v>=ch then
						v=v-ch
						break
					end
					mt=mt*2
					ch=ch-mt
					mode=mode-1
				end
			end
			
			if mode==0 then
				-- Copy the byte
				str2=str2..string.char(v)
			elseif mode==1 then
				-- Encode one byte
				mode=0
				str2=str2..string.format('\\u%04X',v)
			elseif mode==-1 then
				mode=0
			else
				-- Start of various bytes
				current=1
				buffer[current]=v
			end
		else
			current=current+1
			buffer[current]=v-128
			if current==mode then
				cp=0
				for m = 0, mode-1 do
					cp=cp+buffer[mode-m]*(64^m)
				end
				
				str2=str2..string.format('\\u%04X',cp)
				
				mode=0
				current=0
			end
		end
	end
	
	return str2
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
	-- {"content":[["font",{"color":"008080","name":"宋佁E,"size":12,"style":[0,0,0]}],"... "],"from_uin":3132949066,"msg_id":1297,"msg_id2":404816,"msg_type":9,"reply_ip":176881869,"time":1343452203,"to_uin":431533706}
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
		-- r:{"to":3043560234,"group_sig":"b83900ad33607db576d106ffba5ad4e2b8b1cadd660d059efdde23781b3b0c858b5a0452acd28d421af7867d606e4593","face":0,"content":"[\"test2\",\"\\n【提示：此用户正在使用Q+ Web：http://webqq.qq.com/】\",[\"font\",{\"name\":\"宋体\",\"size\":\"10\",\"style\":[0,0,0],\"color\":\"000000\"}]]","msg_id":61830001,"service_type":0,"clientid":"2183284","psessionid":"8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e31313500000156000013e3016e04008aaeb8196d0000000a40746e61464a7232656a6d0000002860a64b417129b223d3ec0759208fd0ef4eb92081e98a37d787a344ba6b23beced3935ef1a52664e0"}
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
				fn=OP_UploadQunImage(s:sub(6))
				if addkey==false then
					addkey=true
					head=head..'"group_code":'..parts[3]..',"key":"'..gface_key..'","sig":"'..gface_sig..'",'
				end
				str2=str2..fn
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
		print('OP_SendMessage: First time send failed, try again')
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
	local url='http://up.web2.qq.com/cgi-bin/cface_upload?time='..GetTS()
	local ret=OP_Post(url,OP_referer_g,'',-1,{from='control',f='EQQ.Model.ChatMsg.callbackSendPicGroup',vfwebqq=vfwebqq,custom_face='\t'..pathname})
	print('result='..ret)
	-- test6[img]D:\Miranda-IM\587D08324C50F0E9584ACA35CA4B13F1.JPG[/img]
	-- <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPicGroup({'ret':0,'msg':'587D08324C50F0E9584ACA35CA4B13F1.jPg'});</script></head><body></body>
	-- <head><script type="text/javascript">document.domain='qq.com';parent.EQQ.Model.ChatMsg.callbackSendPicGroup({'ret':4,'msg':'587D08324C50F0E9584ACA35CA4B13F1.jPg -6102 upload cface failed'});</script></head><body></body>
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
		
		if gface_key==nil then
			-- GFace Key required to send qun image (one time)
			Channel_GetGFaceSig2()
		end
		
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

-- Get contact list
function API_GetUserFriends2()
	local j=CallAPIPost('get_user_friends2',{h='hello'})
	
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
	
	if j~=nil then
		-- {"retcode":0,"result":{"reply":0,"gface_key":"Icn6VvY89nFcaGiH","gface_sig":"a2e96a31386ce2ebac8fe8426b1d0b80a462bbf1c7340bb40508e82b9cbe52c668d7b78b13a2f42fccb882836a7327c2dfc63573ba718279"}}
		print('GFace key acquired')
		gface_key=j.gface_key
		gface_sig=j.gface_sig
	else
		OP_BadReply('get_discu_list_new2')
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
	if tp==4 then svr=1 end
	
	local url='http://face'..svr..'.qun.qq.com/cgi/svr/face/getface?cache=1&type='..tp..'&fid='..fid..'&uin='..tuin..'&vfwebqq='..vfwebqq..'&t='..GetTS()
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
	-- isqun tuin uin fid
	local parts=Split(str,'\t')
	-- local url
	
	if parts[1]=='1' then
		-- Qun
		-- url = 'http://face1.qun.qq.com/cgi/svr/face/getface?cache=0&type=4&fid=0&uin='..tuin..'&vfwebqq='..vfwebqq..'&t='..GetTS()
		FACE_GetFace(tonumber(parts[2]),4,0,tonumber(parts[3]))
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
		-- print('Poll result='..resp)
		
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
								local msg=ProcessMessage(o.content,tuin,gtuin,o.msg_id)
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
								local msg=ProcessMessage(o.content,tuin,did,o.msg_id)
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
						-- {"content":[["font",{"color":"008080","name":"宋佁E,"size":12,"style":[0,0,0]}],"... "],"from_uin":3132949066,"msg_id":1297,"msg_id2":404816,"msg_type":9,"reply_ip":176881869,"time":1343452203,"to_uin":431533706}
						-- {"retcode":0,"result":[{"poll_type":"message","value":{"msg_id":18631,"from_uin":3291940000,"to_uin":431533706,"msg_id2":902526,"msg_type":9,"reply_ip":176498347,"time":1347687817,"content":[["font",{"size":8,"color":"000000","style":[0,0,0],"name":"Tahoma"}],["cface","5E10B9BCA2BDF8489CB3239DFFA745B5.jpg",""]," "]}}]}
-- http://d.web2.qq.com/channel/get_cface2?lcid=18631&guid=5E10B9BCA2BDF8489CB3239DFFA745B5.jpg&to=3291940000&count=5&time=1&clientid=42759885&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000049500001266016e04008aaeb8196d0000000a40504c4847507a4266466d00000028f2ed4db4ddbcc969b0659941493b3795c247e1360ccabc8ab68ec2c35f239b1517b789225706b2ab
-- Referer:http://webqq.qq.com/

						if (v.value.msg_type==9) then
							o=v.value
							if CheckMessageID(o)==true then
								local tuin=o.from_uin
								local tm=o.time
								local msg=ProcessMessage(o.content,tuin,0,o.msg_id)
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
-- i="201" s="s.qun.qq.com/god/images/channelicon.png" c="75797A"/><n t="t" s="邀
--请您一起观看"/><n t="t" s="《活力影院:大块头有大智慧、E/><n t
--="b"/><n t="t" s="00:11:46"/><n t="r"/><n t="i" s="s.qun.qq.com/god/images/chann
--el2084914015.jpg" l="__/air/qqlive"/><n t="r"/><n t="b"/><n t="t" l="__/air/qqli
--ve" s="观看持E͗"/></d>							

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
								local msg=ProcessMessage(o.content,tuin,0,o.msg_id)
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
					else
						print(JSON:encode(j.result))
					end
				end
			elseif j.retcode==116 then
				-- Update ptwebqq
				ptwebqq=j.p
				print('Poll2: ptwebqq updated')
			elseif j.retcode==110 then
				error('ERRLOFF')
			elseif j.retcode==121 then
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

-- Change user status
-- status is Miranda IM status (400xx)
function OP_ChangeStatus(status)
	local statustext=StatusMIM2Text(status)
	local j=CallChannel('change_status2','newstatus='..statustext..'&')
	
	print('Change status to '..statustext);
	
	if j~=nil then
		OP_BadReply('change_status2')
	end
end

-- Get Qun Information by group code (! not tuin!)
function OP_GetGroupInfoExt2(gcode)
	local j=CallAPI('get_group_info_ext2','gcode='..gcode)
	
	-- {"retcode":0,"result":{"stats":[{"client_type":1,"uin":1649606722,"stat":10},{"client_type":41,"uin":431533706,"stat":10}],"minfo":[{"nick":"j85379868","province":"����","gender":"male","uin":1355897758,"country":"����","city":""},{"nick":"^_^","province":"","gender":"male","uin":1649606722,"country":"","city":""},{"nick":"^_^2","province":"","gender":"unknown","uin":431533706,"country":"","city":""}],"ginfo":{"face":0,"memo":"","class":10048,"uin":2138914413,"fingermemo":"","code":4101376875,"createtime":1206845216,"flag":1064961,"level":0,"name":"�����Q2","gid":2523811027,"owner":431533706,"markname":"Notice3_Group","members":[{"muin":1355897758,"mflag":0},{"muin":1649606722,"mflag":73},{"muin":431533706,"mflag":0}],"option":2},"cards":[{"muin":1649606722,"card":"^_^1����"}],"vipinfo":[{"vip_level":0,"u":1355897758,"is_vip":0},{"vip_level":0,"u":1649606722,"is_vip":0},{"vip_level":0,"u":431533706,"is_vip":0}]}}
	
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
		
		-- "ginfo":{"face":0,"memo":"","class":10048,"uin":2138914413,"fingermemo":"","code":4101376875,"createtime":1206845216,"flag":1064961,"level":0,"name":"�����Q2","gid":2523811027,"owner":431533706,"markname":"Notice3_Group","members":[{"muin":1355897758,"mflag":0},{"muin":1649606722,"mflag":73},{"muin":431533706,"mflag":0}],"option":2}
		
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
	
	-- {"retcode":0,"result":{"face":0,"birthday":{"month":0,"year":0,"day":0},"phone":"","occupation":"","allow":1,"college":"","uin":3043560234,"blood":0,"constel":0,"homepage":"","stat":10,"country":"中国","city":"","personal":"","nick":"j85379868","shengxiao":0,"email":"","token":"5c206e43d03e5f6a52cac1d16351db53a33ce7af423b151f","client_type":1,"province":"福建","gender":"male","mobile":"-"}}
	
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
