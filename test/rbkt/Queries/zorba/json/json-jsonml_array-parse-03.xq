import module namespace jx = "http://zorba.io/modules/json-xml";

let $json := '
  [ "file",
    {
      "dir" : "/tmp",
      "name" : "foo",
      "size" : 1234,
      "modified" : "2006-12-31T23:59"
    }
  ]
'
let $options := { "json-format" : "JsonML-array" }
return jx:json-string-to-xml( $json, $options )

(: vim:set et sw=2 ts=2: :)
