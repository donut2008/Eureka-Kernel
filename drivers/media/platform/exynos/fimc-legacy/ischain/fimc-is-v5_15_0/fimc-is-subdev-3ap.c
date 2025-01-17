ined, default is returned, or, if not given,
ValueError is raised.            digit($self, chr, default=<unrepresentable>, /)
--

Converts a Unicode character into its equivalent digit value.

Returns the digit value assigned to the character chr as integer.
If no such value is defined, default is returned, or, if not given,
ValueError is raised.  numeric($self, chr, default=<unrepresentable>, /)
--

Converts a Unicode character into its equivalent numeric value.

Returns the numeric value assigned to the character chr as float.
If no such value is defined, default is returned, or, if not given,
ValueError is raised.              category($self, chr, /)
--

Returns the general category assigned to the character chr as string.               bidirectional($self, chr, /)
--

Returns the bidirectional class assigned to the character chr as string.

If no such value is defined, an empty string is returned.            combining($self, chr, /)
--

Returns the canonical combining class assigned to the character chr as integer.

Returns 0 if no combining class is defined.       mirrored($self, chr, /)
--

Returns the mirrored property assigned to the character chr as integer.

Returns 1 if the character has been identified as a "mirrored"
character in bidirectional text, 0 otherwise.               east_asian_width($self, chr, /)
--

Returns the east asian width assigned to the character chr as string.       decomposition($self, chr, /)
--

Returns the character decomposition mapping assigned to the character chr as string.

An empty string is returned in case no such mapping is defined.          name($self, chr, default=<unrepresentable>, /)
--

Returns the name assigned to the character chr as a string.

If no name is defined, default is returned, or, if not given,
ValueError is raised.             lookup($self, name, /)
--

Look up character by name.

If a character with the given name is found, return the
corresponding character.  If not found, KeyError is raised.      is_normalized($self, form, unistr, /)
--

Return whether the Unicode string unistr is in the normal form 'form'.

Valid values for form are 'NFC', 'NFKC', 'NFD', and 'NFKD'.   normalize($self, form, unistr, /)
--

Return the normal form 'form' for the Unicode string unistr.

Valid values for form are 'NFC', 'NFKC', 'NFD', and 'NFKD'.           	 
                        ! " # $ % & ' ( ) ) ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N N O P Q R S T U V W X Y Z [ \ ] ^ _ ` a b c d e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e f e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e g h e e e e e e e e i ) ) j k l m n o p q r s t u v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v v w x x x x x x x x x x x x x x x x y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y y z z { | } ~   � � � � � � � � � � � � � � � � � � ) ) � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � ) ) ) ) ) ) ) � � ) � � � � � � � � � � � � � � � � � � � � � � ) ) ) ) ) ) ) ) � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � ) ) ) ) � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � ) ) ) ) � � � � � � � � � � � � e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e � e e e e e e e e e � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � e e � e e � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � N � � � � � � � � � � � � � � � N N N N � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e  e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e � � � � � � � � � � � � � � � � � � � � � � � � z z z z � � � � � � � � � � � e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e e � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � �