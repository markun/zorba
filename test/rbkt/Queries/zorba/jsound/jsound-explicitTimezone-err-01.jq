import module namespace jsv = "http://jsound.io/modules/validate"; 

let $jsd :=
  {
    "$namespace" : "http://www.example.com/my-schema",
    "$types" : [
      {
        "$kind" : "atomic",
        "$name" : "foo",
        "$baseType" : "time",
        "$explicitTimezone" : true
        (: must be one of "prohobited", "optional", or "required" :)
      }
    ]
  }

let $instance := xs:time( "11:42:00" )

return jsv:jsd-validate( $jsd, "foo", $instance )

(: vim:set syntax=xquery et sw=2 ts=2: :)
