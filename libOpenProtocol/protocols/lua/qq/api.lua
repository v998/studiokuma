function OP_BadReply(str)
	error('ERRBRPY'..str)
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

function url_encode(str)
  if (str) then
    str = string.gsub (str, "\n", "\r\n")
    str = string.gsub (str, "([^%w ])",
        function (c) return string.format ("%%%02X", string.byte(c)) end)
    str = string.gsub (str, " ", "+")
  end
  return str	
end

function pack(...)
	return { n = select("#", ...), ... }
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

	for k,v in ipairs(pack(string.byte(str,1,string.len(str)))) do
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

-- OP Web Server calls this to fetch Qun Images
HandleQunImage=function(uri)
	print('eqq.HandleQunImage: '..uri)

	-- gid/tuin/tm/server/port/fileid/name
	if vfwebqq==nil then
		print("eqq.HandleQunImage: vfwebqq not set!")
		return
	end

	local parts=Split(uri,"/")
	if #parts==7 then
		-- http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid=1058984205&uin=4200642849&rip=124.115.12.192&rport=8000&fid=1799487874&pic=%7B4E3FF3AA-C100-E536-6CE3-6E53427DF5F6%7D.jpg&vfwebqq=1aa616dedcc8f8214e83b28704960dd2233cae8f3c800980eea50e01e93768e8b32af25cffe14342&t=1343641932
		local url="http://webqq.qq.com/cgi-bin/get_group_pic?type=0&gid="..parts[1].."&uin="..parts[2].."&rip="..parts[4].."&rport="..parts[5].."&fid="..parts[6].."&pic="..parts[7].."&vfwebqq="..vfwebqq.."&t="..parts[3]
		local referer="http://webqq.qq.com/"
		print('eqq.HandleQunImage: Fetching '..parts[7]..' from qun '..parts[1]..'...')
		
		local img=OP_Get(url,referer,true)
		if (string.len(img)==0) then
			print('Fetch failed, try again')
			img=OP_Get(url,referer)
		end
		
		if (string.len(img)~=0) then
			print("eqq.HandleQunImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[7]
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("eqq.HandleQunImage: GET failed for "..parts[7])
		end
	else
		print("eqq.HandleQunImage: Invalid input count: "..#parts..' uri='..uri)
		
		for k3,v3 in ipairs(parts) do
			print(" #"..k3.."="..v3)
		end
	end
end

-- OP Web Server calls this to fetch P2P Images
HandleP2PImage=function(uri)
	-- tuin/msgid/name
	-- tuin/filename
	if clientid==nil or psessionid==nil then
		print("eqq.HandleP2PImage: clientid or psessionid not set!")
		return
	end

	local parts=Split(uri,"/")
	if #parts==3 then
		-- http://d.web2.qq.com/channel/get_cface2?lcid=18631&guid=5E10B9BCA2BDF8489CB3239DFFA745B5.jpg&to=3291940000&count=5&time=1&clientid=42759885&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000049500001266016e04008aaeb8196d0000000a40504c4847507a4266466d00000028f2ed4db4ddbcc969b0659941493b3795c247e1360ccabc8ab68ec2c35f239b1517b789225706b2ab
		local url="http://d.web2.qq.com/channel/get_cface2?lcid="..parts[2].."&guid="..parts[3].."&to="..parts[1].."&count=5&time=1&clientid="..clientid.."&psessionid="..psessionid
		local referer=OP_referer_g
		print('eqq.HandleP2PImage: Fetching '..parts[3]..' from tuin '..parts[1]..'...')
		local img=OP_Get(url,referer)
		if (string.len(img)~=0) then
			print("eqq.HandleP2PImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[1]..'_'..parts[2]..'_'..parts[3]
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("eqq.HandleP2PImage: GET failed for "..parts[3])
		end
	elseif #parts==2 then
		-- http://d.web2.qq.com/channel/get_offpic2?file_path=%2F0fb2dca1-72bf-4fce-9da8-3a34f7b2d217&f_uin=1263314672&clientid=8624094&psessionid=8368046764001e636f6e6e7365727665725f77656271714031302e3132382e36362e3131350000369a00001a56016e04008aaeb8196d0000000a406b51516353535846616d000000288d8af4a0349faef00e36d3eb0fe0d96bae908dcdd4c677f20ff03cccdcb37b631b2dc9668392ded6
		parts[2]=parts[2]:sub(1,-5)
		local url="http://d.web2.qq.com/channel/get_offpic2?file_path=%2F"..parts[2].."&f_uin="..parts[1].."&clientid="..clientid.."&psessionid="..psessionid
		local referer=OP_referer_g
		print('eqq.HandleP2PImage: Fetching '..parts[2]..' from tuin '..parts[1]..'...')
		local img=OP_Get(url,referer)
		if (string.len(img)~=0) then
			print("eqq.HandleP2PImage: GET url="..url.." size="..string.len(img))
			local tempfile=OP_qunimagedir..'/'..parts[1]..'_'..parts[2]..'.jpg'
			local fh=io.open(tempfile,"wb")
			fh:write(img)
			fh:close()
		else
			print("eqq.HandleP2PImage: GET failed for "..parts[3])
		end
	else
		print("eqq.HandleP2PImage: Invalid input count: "..#parts..' uri='..uri)
		
		for k3,v3 in ipairs(parts) do
			print(" #"..k3.."="..v3)
		end
	end
end

OP_GetAvatar=function(str)
	local getFaceServer = function (b)
		return AVATAR_SERVER_DOMAINS[(b%8)+1]
	end -- func getFaceServer
	
	local getUserAvatar = function (b, i, a)
		i = "&vfwebqq="..vfwebqq
		if a~=nil then i = "" end
		return getFaceServer(b).."cgi/svr/face/getface?cache=0&type=11&fid=0&uin="..b..i
	end -- func getUserAvatar
	
	local getGroupAvatar = function (b)
		return getFaceServer(b).."cgi/svr/face/getface?cache=0&type=14&fid=0&uin="..b.."&vfwebqq="..vfwebqq
	end -- func getGroupAvatar
	
	-- isqun tuin uin code
	local parts=Split(str,'\t')
	local url
	local tuin
	
	if parts[1]=='1' then
		-- Qun
		tuin=tonumber(parts[4])
		url=getGroupAvatar(tuin) -- parts[4]==group_code
	else
		-- User
		tuin=tonumber(parts[2])
		url=getUserAvatar(tuin )
	end
	
	print ('OP_GetAvatar: url='..url)
	local uin=tonumber(parts[3])
	if uin==nil then uin=tuin end -- Shoule be impossible

	local resp=OP_Get(url,referer_s)
	local fn=OP_avatardir..'/'..uin..'.jpg'
	local fh=io.open(fn,"wb")
	if fh==nil then
		print('OP_GetAvatar: Failed opening '..fn..' for writing!')
	else
		fh:write(resp)
		fh:close()
	end
	
	OP_UpdateAvatar(tuin,fn)
end -- OP_GetAvatar

-- Map Miranda IM status to QQ status text
local StatusMIM2Text=function(st)
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

-- Change user status
-- status is Miranda IM status (400xx)
function OP_ChangeStatus(status)
	sendChangeStatus({newstatus=StatusMIM2Text(status)})
end
