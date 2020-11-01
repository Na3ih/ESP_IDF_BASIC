#pragma once
#include <stdio.h>

const static char htmlExample[] = " \
<!DOCTYPE html> \
<html lang=""""en""> \
<head> \
    <meta charset=""UTF-8""> \
    <meta name=""viewport"" content=""width=device-width, initial-scale=1.0""> \
    <title>Formularz</title> \
</head> \
<body> \
    <form action=""mailto:email@@wp.pl"" method=""post"">   \
        <fieldset>    \
            <legend>Dane osobowe</legend>  \
            <label for=""name_id""> Imię: </label> \
            <input type=""text"" name=""imie"" id=""name_id"">    \
            <br />   \
            <label for=""surname_id""> Nazwisko: </label>  \
            <input type=""text"" name=""nazwisko"" id=""surname_id""> \
            <br />   \
            <label for=""kobieta"">Kobieta</label> \
            <input type=""radio"" name=""plec"" id=""kobieta"">   \
            <br />   \
            <label for=""mezczyzna"">Mężczyzna</label> \
            <input type=""radio"" name=""plec"" id=""mezczyzna""> \
        </fieldset>  \
        <br />   \
        <fieldset>    \
            <legend>Twoje pytanie</legend> \
            <label for=""adresat"">Adresat</label> \
            <select name=""Adresat"" id=""adresat"">  \
                <optgroup label = Dział sprzedaży>    \
                <option value=""0"">Michał</option>    \
                </optgroup>  \
                <optgroup label=""Serwis"">   \
                    <option value=""1"">Marek</option> \
                    <option value=""2"">Zenek</option> \
                </optgroup>  \
            </select>    \
            <br />   \
            <textarea name=""pytanie"" cols=""30"" rows=""10""></textarea> \
        <fieldset>    \
    </form>  \
</body>  \
</html>  \
";