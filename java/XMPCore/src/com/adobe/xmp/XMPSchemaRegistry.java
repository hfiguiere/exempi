// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

package com.adobe.xmp;

import java.util.Map;

import com.adobe.xmp.options.AliasOptions;
import com.adobe.xmp.properties.XMPAliasInfo;

/**
 * The schema registry keeps track of all namespaces and aliases used in the XMP
 * metadata. At initialisation time, the default namespaces and default aliases
 * are automatically registered. <b>Namespaces</b> must be registered before
 * used in namespace URI parameters or path expressions. Within the XMP Toolkit
 * the registered namespace URIs and prefixes must be unique. Additional
 * namespaces encountered when parsing RDF are automatically registered. The
 * namespace URI should always end in an XML name separator such as '/' or '#'.
 * This is because some forms of RDF shorthand catenate a namespace URI with an
 * element name to form a new URI.
 * <p>
 * <b>Aliases</b> in XMP serve the same purpose as Windows file shortcuts,
 * Macintosh file aliases, or UNIX file symbolic links. The aliases are simply
 * multiple names for the same property. One distinction of XMP aliases is that
 * they are ordered, there is an alias name pointing to an actual name. The
 * primary significance of the actual name is that it is the preferred name for
 * output, generally the most widely recognized name.
 * <p>
 * The names that can be aliased in XMP are restricted. The alias must be a top
 * level property name, not a field within a structure or an element within an
 * array. The actual may be a top level property name, the first element within
 * a top level array, or the default element in an alt-text array. This does not
 * mean the alias can only be a simple property. It is OK to alias a top level
 * structure or array to an identical top level structure or array, or to the
 * first item of an array of structures.
 * 
 * @since 27.01.2006
 */
public interface XMPSchemaRegistry
{
	// ---------------------------------------------------------------------------------------------
	// Namespace Functions

	/**
	 * Register a namespace URI with a suggested prefix. It is not an error if
	 * the URI is already registered, no matter what the prefix is. If the URI
	 * is not registered but the suggested prefix is in use, a unique prefix is
	 * created from the suggested one. The actual registeed prefix is always
	 * returned. The function result tells if the registered prefix is the
	 * suggested one.
	 * <p>
	 * Note: No checking is presently done on either the URI or the prefix.
	 * 
	 * @param namespaceURI
	 *            The URI for the namespace. Must be a valid XML URI.
	 * @param suggestedPrefix
	 *            The suggested prefix to be used if the URI is not yet
	 *            registered. Must be a valid XML name.
	 * @return Returns the registered prefix for this URI, is equal to the
	 *         suggestedPrefix if the namespace hasn't been registered before,
	 *         otherwise the existing prefix.
	 * @throws XMPException If the parameters are not accordingly set
	 */
	String registerNamespace(String namespaceURI, String suggestedPrefix) throws XMPException;

	
	/**
	 * Obtain the prefix for a registered namespace URI.
	 * <p>
	 * It is not an error if the namespace URI is not registered. The output
	 * namespacePrefix string is not modified if the namespace URI is not
	 * registered.
	 * 
	 * @param namespaceURI
	 *            The URI for the namespace. Must not be null or the empty
	 *            string.
	 * @return Returns true if the namespace URI is registered.
	 */
	String getNamespacePrefix(String namespaceURI);

	
	/**
	 * Obtain the URI for a registered namespace prefix.
	 * <p>
	 * It is not an error if the namespace prefix is not registered. The output
	 * namespaceURI string is not modified if the namespace prefix is not
	 * registered.
	 * 
	 * @param namespacePrefix
	 *            The prefix for the namespace. Must not be null or the empty
	 *            string.
	 * @return Returns the URI registered for this prefix.
	 */
	String getNamespaceURI(String namespacePrefix);

	
	/**
	 * @return Returns the registered prefix/namespace-pairs as map, where the keys are the
	 *         namespaces and the values are the prefixes.
	 */
	Map getNamespaces();

	
	/**
	 * @return Returns the registered namespace/prefix-pairs as map, where the keys are the
	 *         prefixes and the values are the namespaces.
	 */
	Map getPrefixes();
	
	
	/**
	 * Deletes a namespace from the registry.
	 * <p>
	 * Does nothing if the URI is not registered, or if the namespaceURI
	 * parameter is null or the empty string.
	 * <p>
	 * Note: Not yet implemented.
	 * 
	 * @param namespaceURI
	 *            The URI for the namespace.
	 */
	void deleteNamespace(String namespaceURI);

	
	
	
	
	// ---------------------------------------------------------------------------------------------
	// Alias Functions

	/**
	 * Associates an alias name with an actual name.
	 * <p>
	 * Define a alias mapping from one namespace/property to another. Both
	 * property names must be simple names. An alias can be a direct mapping,
	 * where the alias and actual have the same data type. It is also possible
	 * to map a simple alias to an item in an array. This can either be to the
	 * first item in the array, or to the 'x-default' item in an alt-text array.
	 * Multiple alias names may map to the same actual, as long as the forms
	 * match. It is a no-op to reregister an alias in an identical fashion.
	 * 
	 * @param aliasNS
	 *            The namespace URI for the alias. Must not be null or the empty
	 *            string.
	 * @param aliasProp
	 *            The name of the alias. Must be a simple name, not null or the
	 *            empty string and not a general path expression.
	 * @param actualNS
	 *            The namespace URI for the actual. Must not be null or the
	 *            empty string.
	 * @param actualProp
	 *            The name of the actual. Must be a simple name, not null or the
	 *            empty string and not a general path expression.
	 * @param aliasForm
	 *            Provides options for aliases for simple aliases to array
	 *            items. This is needed to know what kind of array to create if
	 *            set for the first time via the simple alias. Pass
	 *            <code>XMP_NoOptions</code>, the default value, for all
	 *            direct aliases regardless of whether the actual data type is
	 *            an array or not (see {@link AliasOptions}).
	 * @throws XMPException
	 *             for inconsistant aliases.
	 */
	void registerAlias(String aliasNS, String aliasProp, String actualNS, String actualProp,
			AliasOptions aliasForm) throws XMPException;

	
	/**
	 * Determines if a name is an alias, and what it is aliased to.
	 * 
	 * @param aliasNS
	 *            The namespace URI of the alias. Must not be <code>null</code> or the empty
	 *            string.
	 * @param aliasProp
	 *            The name of the alias. May be an arbitrary path expression
	 *            path, must not be <code>null</code> or the empty string.
	 * @return Returns the <code>XMPAliasInfo</code> for the given alias namespace and property or
	 * 		<code>null</code> if there is no such alias.
	 */
	XMPAliasInfo resolveAlias(String aliasNS, String aliasProp);

	
	/**
	 * Collects all aliases that are contained in the provided namespace.
	 * If nothing is found, an empty array is returned. 
	 * 
	 * @param aliasNS a schema namespace URI
	 * @return Returns all alias infos from aliases that are contained in the provided namespace. 
	 */
	XMPAliasInfo[] findAliases(String aliasNS);
	
	
	/**
	 * Searches for registered aliases.
	 * 
	 * @param qname
	 *            an XML conform qname
	 * @return Returns if an alias definition for the given qname to another
	 *         schema and property is registered.
	 */
	XMPAliasInfo findAlias(String qname);
	
		
	/**
	 * Delete an alias.
	 * <p>
	 * This only deletes the registration of the alias, it does not delete the
	 * actual property. It does delete any view of the property through the
	 * alias name. It is OK to attempt to delete an alias that does not exist,
	 * that is if the alias name is not registered as an alias.
	 * 
	 * @param aliasNS
	 *            The namespace URI for the alias. Must not be null or the empty
	 *            string.
	 * @param aliasProp
	 *            The name of the alias. Must be a simple name, not null or the
	 *            empty string and not a general path expression.
	 */
	void deleteAlias(String aliasNS, String aliasProp);

	
	/**
	 * @return Returns the registered aliases as map, where the key is the "qname" (prefix and name)
	 * and the value an <code>XMPAliasInfo</code>-object.
	 */
	Map getAliases();


}