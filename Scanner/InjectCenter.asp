
<html>

<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312">
<meta name="GENERATOR" content="Microsoft FrontPage 6.0">
<meta name="ProgId" content="FrontPage.Editor.Document">
<title>SQLע��ϵͳ��ʾ����</title>
</head>

<body>
<div align="center">

<table border="1" width="100%" height="424">
  <tr>
    <td width="100%" bgcolor="#008080" height="32">
      <p align="center"><font color="#FFFF00"><b>����ϵͳ</b></font></p>
    </td>
  </tr>
  <center>

  <tr>
<td height="351" align="left" bgcolor="#FFFFFF" valign="top">

<font size="2" color="#0000FF">
<p align="center"><br><br><br>
<%
Usernamef=request.form("Username")
Passwordf=request.form("Password")
Usernamer=request("Username")
Passwordr=request("Password")

set conn=server.CreateObject("ADODB.Connection")
'set rs=server.CreateObject("ADODB.RecordSet")
connstr="Driver={SQL Server};server=192.168.1.104;uid=sa;pwd=sql123..;database=SQLScan;"
conn.open connstr

if Usernamef<>"" then
	sql="select * from UserList where [User]='"+Usernamef+"' and Pass='"+Passwordf+"'"
else
	sql="select * from UserList where [User]='"+Usernamer+"' and Pass='"+Passwordr+"'"
end if
response.write sql+"<br>"
set rs=conn.execute(sql)

if not rs.EOF then
	response.write "�û����ͨ������ӭ����ϵͳ��"
else
	response.write "�û������벻�ԣ����������롣"
end if

rs.close
set rs=nothing
conn.close
set conn=nothing
%>

 ��</font>
<br><br><br>
  <font color="#FFFF00" size="2">[<a href="InjectTest.asp">���ص�¼��ҳ</a>]</font></tr>
  <tr>
    <td width="100%" bgcolor="#008080" height="23" align="center">
      <p align="right"><font color="#FFFF00" size="2">Copyright(c) 2020, All rights reserved.</font></p>       
    </td>
  </tr>
</table>

  </center>
</div>

</body>

</html>
