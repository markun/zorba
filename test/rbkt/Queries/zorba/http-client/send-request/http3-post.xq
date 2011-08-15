import module namespace http = "http://www.zorba-xquery.com/modules/http-client";

declare default element namespace "http://expath.org/ns/http-client";

variable $req :=
  <request method="POST" href="http://www.zorba-xquery.com/rest-tests/request.php">
    <header name="foo" value="bar"/>
    <body media-type="text/plain">
      Dies ist ein kleiner Test
    </body>
  </request>;

variable $http-res := http:send-request($req, (), ());

$http-res[2]

