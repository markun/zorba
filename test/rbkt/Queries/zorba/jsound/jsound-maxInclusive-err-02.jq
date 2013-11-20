import module namespace jsv = "http://jsound.io/modules/validate"; 

let $jsd :=
  {
    "$namespace" : "http://www.example.com/my-schema",
    "$types" : [
      {
        "$kind" : "atomic",
        "$name" : "foo",
        "$baseType" : "integer",
        "$maxInclusive" : "bar" (: must be integer :)
      }
    ]
  }

let $instance := 12345

return jsv:jsd-validate( $jsd, "foo", $instance )

(: vim:set syntax=xquery et sw=2 ts=2: :)
