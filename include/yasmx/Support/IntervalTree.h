#ifndef YASM_INTERVALTREE_H
#define YASM_INTERVALTREE_H

//  The interval_tree.h and interval_tree.cc files contain code for
//  interval trees implemented using red-black-trees as described in
//  the book _Introduction_To_Algorithms_ by Cormen, Leisserson,
//  and Rivest.

/* If the symbol YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS is defined then the
 * code does a lot of extra checking to make sure certain assumptions
 * are satisfied.  This only needs to be done if you suspect bugs are
 * present or if you make significant changes and want to make sure
 * your changes didn't mess anything up.
 */
//#define YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS 1

#include <cassert>
#include <climits>
#include <cstdlib>
#include <vector>

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/functional.h"


namespace yasm
{

template <typename T> class IntervalTree;

template <typename T>
class IntervalTreeNode
{
    friend class IntervalTree<T>;

public:
    IntervalTreeNode() {}
    IntervalTreeNode(long l, long h);
    IntervalTreeNode(long l, long h, T d);
    ~IntervalTreeNode() {}

    const T& getData() const { return data; }

protected:
    void Put(raw_ostream& os, IntervalTreeNode<T>* nil,
             IntervalTreeNode<T>* root) const;

    IntervalTreeNode<T>* left;
    IntervalTreeNode<T>* right;
    IntervalTreeNode<T>* parent;
    T data;
    long low;
    long high;
    long maxHigh;
    bool red;  // if red=false then the node is black
};

template <typename T>
class IntervalTree
{
    template <typename U> friend
    raw_ostream& operator<< (raw_ostream& os, const IntervalTree<U>& it);

public:
    IntervalTree();
    ~IntervalTree();
    T DeleteNode(IntervalTreeNode<T>*, long& low, long& high);
    IntervalTreeNode<T>* Insert(long low, long high, T data);
    IntervalTreeNode<T>* getPredecessor(IntervalTreeNode<T>*) const;
    IntervalTreeNode<T>* getSuccessor(IntervalTreeNode<T>*) const;
    void Enumerate(long low, long high,
                   TR1::function<void (IntervalTreeNode<T>*)> callback);
    void Put(raw_ostream& os) const;
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    void CheckAssumptions() const;
#endif

protected:
    class RecursionNode
    {
    public:
        RecursionNode() : start_node(0), parentIndex(0), tryRightBranch(false)
        {}
        RecursionNode(IntervalTreeNode<T>* sn, unsigned int pi, bool tr)
            : start_node(sn), parentIndex(pi), tryRightBranch(tr)
        {}
        // this structure stores the information needed when we take the
        // right branch in searching for intervals but possibly come back
        // and check the left branch as well.
        IntervalTreeNode<T>* start_node;
        unsigned int parentIndex;
        bool tryRightBranch;
    };

    // A sentinel is used for root and for nil.  These sentinels are
    // created when ITTreeCreate is caled.  root->left should always
    // point to the node which is the root of the tree.  nil points to a
    // node which should always be black but has arbitrary children and
    // parent and no key or info.  The point of using these sentinels is so
    // that the root and nil nodes do not require special cases in the code
    IntervalTreeNode<T>* m_root;
    IntervalTreeNode<T>* m_nil;

    void LeftRotate(IntervalTreeNode<T>*);
    void RightRotate(IntervalTreeNode<T>*);
    void TreeInsertHelp(IntervalTreeNode<T>*);
    void Put(raw_ostream& os, IntervalTreeNode<T>*) const;
    void FixUpMaxHigh(IntervalTreeNode<T>*);
    void DeleteFixUp(IntervalTreeNode<T>*);

    static long Max(long a, long b);

#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    void CheckMaxHighFields(IntervalTreeNode<T>*) const;
    bool CheckMaxHighFieldsHelper(IntervalTreeNode<T>* y,
                                  long currentHigh,
                                  bool match) const;

    static void Verify(bool condition, const char* condstr, const char* file,
                       unsigned long line);
#endif
};

template <typename U>
inline raw_ostream&
operator<< (raw_ostream& os, const IntervalTree<U>& it)
{
    it.Put(os, it.m_root->left);
    return os;
}

// a function to find the maximum of two objects.
template <typename T>
inline long
IntervalTree<T>::Max(long a, long b)
{
    return ((a>b) ? a : b);
}

/***********************************************************************/
/*  FUNCTION:  Overlap */
/**/
/*    INPUTS:  [a1,a2] and [b1,b2] are the low and high endpoints of two */
/*             closed intervals.  */
/**/
/*    OUTPUT:  stack containing pointers to the nodes between [low,high] */
/**/
/*    Modifies Input: none */
/**/
/*    EFFECT:  returns 1 if the intervals overlap, and 0 otherwise */
/***********************************************************************/

inline bool
Overlap(int a1, int a2, int b1, int b2)
{
    if (a1 <= b1)
        return (b1 <= a2);
    else
        return (a1 <= b2);
}

} // anonymous namespace

namespace yasm
{

template <typename T>
IntervalTreeNode<T>::IntervalTreeNode(long l, long h)
    : data(0)
{
    if (l < h)
    {
        low = l;
        high = h;
    }
    else
    {
        low = h;
        high = l;
    }
    maxHigh = h;
}

template <typename T>
IntervalTreeNode<T>::IntervalTreeNode(long l, long h, T d)
    : data(d)
{
    if (l < h)
    {
        low = l;
        high = h;
    }
    else
    {
        low = h;
        high = l;
    }
    maxHigh = h;
}

template <typename T>
IntervalTree<T>::IntervalTree()
{
    m_nil = new IntervalTreeNode<T>(LONG_MIN, LONG_MIN);
    m_nil->left = m_nil;
    m_nil->right = m_nil;
    m_nil->parent = m_nil;
    m_nil->red = false;

    m_root = new IntervalTreeNode<T>(LONG_MAX, LONG_MAX);
    m_root->left = m_nil;
    m_root->right = m_nil;
    m_root->parent = m_nil;
    m_root->red = false;
}

/***********************************************************************/
/*  FUNCTION:  LeftRotate */
/**/
/*  INPUTS:  the node to rotate on */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input: this, x */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. Also updates the maxHigh fields of x and y */
/*            after rotation. */
/***********************************************************************/

template <typename T>
void
IntervalTree<T>::LeftRotate(IntervalTreeNode<T>* x)
{
    IntervalTreeNode<T>* y;

    /* I originally wrote this function to use the sentinel for
     * nil to avoid checking for nil.  However this introduces a
     * very subtle bug because sometimes this function modifies
     * the parent pointer of nil.  This can be a problem if a
     * function which calls LeftRotate also uses the nil sentinel
     * and expects the nil sentinel's parent pointer to be unchanged
     * after calling this function.  For example, when DeleteFixUP
     * calls LeftRotate it expects the parent pointer of nil to be
     * unchanged.
     */

    y=x->right;
    x->right=y->left;

    if (y->left != m_nil)
        y->left->parent=x; /* used to use sentinel here */
    /* and do an unconditional assignment instead of testing for nil */

    y->parent=x->parent;

    /* Instead of checking if x->parent is the root as in the book, we
     * count on the root sentinel to implicitly take care of this case
     */
    if (x == x->parent->left)
        x->parent->left=y;
    else
        x->parent->right=y;
    y->left=x;
    x->parent=y;

    x->maxHigh=Max(x->left->maxHigh, Max(x->right->maxHigh, x->high));
    y->maxHigh=Max(x->maxHigh, Max(y->right->maxHigh, y->high));
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    CheckAssumptions();
#else
    assert(!m_nil->red);
    assert(m_nil->maxHigh==LONG_MIN);
#endif
}


/***********************************************************************/
/*  FUNCTION:  RightRotate */
/**/
/*  INPUTS:  node to rotate on */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input?: this, y */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. Also updates the maxHigh fields of x and y */
/*            after rotation. */
/***********************************************************************/


template <typename T>
void
IntervalTree<T>::RightRotate(IntervalTreeNode<T>* y)
{
    IntervalTreeNode<T>* x;

    /* I originally wrote this function to use the sentinel for
     * nil to avoid checking for nil.  However this introduces a
     * very subtle bug because sometimes this function modifies
     * the parent pointer of nil.  This can be a problem if a
     * function which calls LeftRotate also uses the nil sentinel
     * and expects the nil sentinel's parent pointer to be unchanged
     * after calling this function.  For example, when DeleteFixUP
     * calls LeftRotate it expects the parent pointer of nil to be
     * unchanged.
     */

    x=y->left;
    y->left=x->right;

    if (m_nil != x->right)
        x->right->parent=y; /*used to use sentinel here */
    /* and do an unconditional assignment instead of testing for nil */

    /* Instead of checking if x->parent is the root as in the book, we
     * count on the root sentinel to implicitly take care of this case
     */
    x->parent=y->parent;
    if (y == y->parent->left)
        y->parent->left=x;
    else
        y->parent->right=x;
    x->right=y;
    y->parent=x;

    y->maxHigh=Max(y->left->maxHigh, Max(y->right->maxHigh, y->high));
    x->maxHigh=Max(x->left->maxHigh, Max(y->maxHigh, x->high));
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    CheckAssumptions();
#else
    assert(!m_nil->red);
    assert(m_nil->maxHigh==LONG_MIN);
#endif
}

/***********************************************************************/
/*  FUNCTION:  TreeInsertHelp  */
/**/
/*  INPUTS:  z is the node to insert */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  this, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  This funciton is only intended to be called */
/*            by the InsertTree function and not by the user */
/***********************************************************************/

template <typename T>
void
IntervalTree<T>::TreeInsertHelp(IntervalTreeNode<T>* z)
{
    /*  This function should only be called by InsertITTree (see above) */
    IntervalTreeNode<T>* x;
    IntervalTreeNode<T>* y;

    z->left=z->right=m_nil;
    y=m_root;
    x=m_root->left;
    while( x != m_nil)
    {
        y=x;
        if (x->low > z->low)
            x=x->left;
        else /* x->low <= z->low */
            x=x->right;
    }
    z->parent=y;
    if ((y == m_root) || (y->low > z->low))
        y->left=z;
    else
        y->right=z;

    assert(!m_nil->red);
    assert(m_nil->maxHigh==LONG_MIN);
}


/***********************************************************************/
/*  FUNCTION:  FixUpMaxHigh  */
/**/
/*  INPUTS:  x is the node to start from*/
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  this */
/**/
/*  EFFECTS:  Travels up to the root fixing the maxHigh fields after */
/*            an insertion or deletion */
/***********************************************************************/

template <typename T>
void
IntervalTree<T>::FixUpMaxHigh(IntervalTreeNode<T>* x)
{
    while(x != m_root)
    {
        x->maxHigh=Max(x->high, Max(x->left->maxHigh, x->right->maxHigh));
        x=x->parent;
    }
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    CheckAssumptions();
#endif
}

/*  Before calling InsertNode  the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  InsertNode */
/**/
/*  INPUTS:  newInterval is the interval to insert*/
/**/
/*  OUTPUT:  This function returns a pointer to the newly inserted node */
/*           which is guarunteed to be valid until this node is deleted. */
/*           What this means is if another data structure stores this */
/*           pointer then the tree does not need to be searched when this */
/*           is to be deleted. */
/**/
/*  Modifies Input: tree */
/**/
/*  EFFECTS:  Creates a node node which contains the appropriate key and */
/*            info pointers and inserts it into the tree. */
/***********************************************************************/

template <typename T>
IntervalTreeNode<T>*
IntervalTree<T>::Insert(long low, long high, T data)
{
    IntervalTreeNode<T> *x, *y, *newNode;

    x = new IntervalTreeNode<T>(low, high, data);
    TreeInsertHelp(x);
    FixUpMaxHigh(x->parent);
    newNode = x;
    x->red=true;
    while(x->parent->red) // use sentinel instead of checking for root
    {
        if (x->parent == x->parent->parent->left)
        {
            y=x->parent->parent->right;
            if (y->red)
            {
                x->parent->red=false;
                y->red=false;
                x->parent->parent->red=true;
                x=x->parent->parent;
            }
            else
            {
                if (x == x->parent->right)
                {
                    x=x->parent;
                    LeftRotate(x);
                }
                x->parent->red=false;
                x->parent->parent->red=true;
                RightRotate(x->parent->parent);
            }
        }
        else // case for x->parent == x->parent->parent->right
        {
            // this part is just like the section above with
            // left and right interchanged
            y=x->parent->parent->left;
            if (y->red)
            {
                x->parent->red=false;
                y->red=false;
                x->parent->parent->red=true;
                x=x->parent->parent;
            }
            else
            {
                if (x == x->parent->left)
                {
                    x=x->parent;
                    RightRotate(x);
                }
                x->parent->red=false;
                x->parent->parent->red=true;
                LeftRotate(x->parent->parent);
            }
        }
    }
    m_root->left->red=false;

#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    CheckAssumptions();
#else
    assert(!m_nil->red);
    assert(!m_root->red);
    assert(m_nil->maxHigh==LONG_MIN);
#endif
    return newNode;
}

/***********************************************************************/
/*  FUNCTION:  GetSuccessorOf  */
/**/
/*    INPUTS:  x is the node we want the succesor of */
/**/
/*    OUTPUT:  This function returns the successor of x or NULL if no */
/*             successor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

template <typename T>
IntervalTreeNode<T>*
IntervalTree<T>::getSuccessor(IntervalTreeNode<T>* x) const
{
    IntervalTreeNode<T>* y;

    if (m_nil != (y = x->right)) // assignment to y is intentional
    {
        while(y->left != m_nil) // returns the minium of the right subtree of x
            y=y->left;
        return y;
    }
    else
    {
        y=x->parent;
        while(x == y->right) // sentinel used instead of checking for nil
        {
            x=y;
            y=y->parent;
        }
        if (y == m_root)
            return(m_nil);
        return y;
    }
}

/***********************************************************************/
/*  FUNCTION:  GetPredecessorOf  */
/**/
/*    INPUTS:  x is the node to get predecessor of */
/**/
/*    OUTPUT:  This function returns the predecessor of x or NULL if no */
/*             predecessor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

template <typename T>
IntervalTreeNode<T>*
IntervalTree<T>::getPredecessor(IntervalTreeNode<T>* x) const
{
    IntervalTreeNode<T>* y;

    if (m_nil != (y = x->left)) /* assignment to y is intentional */
    {
        while(y->right != m_nil) /* returns the maximum of the left subtree of x */
            y=y->right;
        return y;
    }
    else
    {
        y=x->parent;
        while(x == y->left)
        {
            if (y == m_root)
                return(m_nil);
            x=y;
            y=y->parent;
        }
        return y;
    }
}

/***********************************************************************/
/*  FUNCTION:  Print */
/**/
/*    INPUTS:  none */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively prints the nodes of the tree */
/*              inorder. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:    This function should only be called from ITTreePrint */
/***********************************************************************/

template <typename T>
void
IntervalTreeNode<T>::Put(raw_ostream& os,
                         IntervalTreeNode<T>* nil,
                         IntervalTreeNode<T>* root) const
{
    os << ", l=" << low << ", h=" << high << ", mH=" << maxHigh;
    os << "  l->low=";
    if (left == nil)
        os << "NULL";
    else
        os << left->low;
    os << "  r->low=";
    if (right == nil)
        os << "NULL";
    else
        os << right->low;
    os << "  p->low=";
    if (parent == root)
        os << "NULL";
    else
        os << parent->low;
    os << "  red=" << red << '\n';
}

template <typename T>
void
IntervalTree<T>::Put(raw_ostream& os, IntervalTreeNode<T>* x) const
{
    if (x != m_nil)
    {
        Put(os, x->left);
        x->Put(os, m_nil, m_root);
        Put(os, x->right);
    }
}

template <typename T>
void
IntervalTree<T>::Put(raw_ostream& os) const
{
    Put(os, m_root->left);
}

template <typename T>
IntervalTree<T>::~IntervalTree()
{
    IntervalTreeNode<T>* x = m_root->left;
    std::vector<IntervalTreeNode<T>*> stuffToFree;

    if (x != m_nil)
    {
        if (x->left != m_nil)
            stuffToFree.push_back(x->left);
        if (x->right != m_nil)
            stuffToFree.push_back(x->right);
        delete x;
        while (!stuffToFree.empty())
        {
            x = stuffToFree.back();
            stuffToFree.pop_back();

            if (x->left != m_nil)
                stuffToFree.push_back(x->left);
            if (x->right != m_nil)
                stuffToFree.push_back(x->right);
            delete x;
        }
    }

    delete m_nil;
    delete m_root;
}

/***********************************************************************/
/*  FUNCTION:  DeleteFixUp */
/**/
/*    INPUTS:  x is the child of the spliced */
/*             out node in DeleteNode. */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Performs rotations and changes colors to restore red-black */
/*             properties after a node is deleted */
/**/
/*    Modifies Input: this, x */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

template <typename T>
void
IntervalTree<T>::DeleteFixUp(IntervalTreeNode<T>* x)
{
    IntervalTreeNode<T>* w;
    IntervalTreeNode<T>* rootLeft = m_root->left;

    while ((!x->red) && (rootLeft != x))
    {
        if (x == x->parent->left)
        {
            w=x->parent->right;
            if (w->red)
            {
                w->red=false;
                x->parent->red=true;
                LeftRotate(x->parent);
                w=x->parent->right;
            }
            if ( (!w->right->red) && (!w->left->red) )
            {
                w->red=true;
                x=x->parent;
            }
            else
            {
                if (!w->right->red)
                {
                    w->left->red=false;
                    w->red=true;
                    RightRotate(w);
                    w=x->parent->right;
                }
                w->red=x->parent->red;
                x->parent->red=false;
                w->right->red=false;
                LeftRotate(x->parent);
                x=rootLeft; // this is to exit while loop
            }
        }
        else // the code below is has left and right switched from above
        {
            w=x->parent->left;
            if (w->red)
            {
                w->red=false;
                x->parent->red=true;
                RightRotate(x->parent);
                w=x->parent->left;
            }
            if ((!w->right->red) && (!w->left->red))
            {
                w->red=true;
                x=x->parent;
            }
            else
            {
                if (!w->left->red)
                {
                    w->right->red=false;
                    w->red=true;
                    LeftRotate(w);
                    w=x->parent->left;
                }
                w->red=x->parent->red;
                x->parent->red=false;
                w->left->red=false;
                RightRotate(x->parent);
                x=rootLeft; // this is to exit while loop
            }
        }
    }
    x->red=false;

#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
    CheckAssumptions();
#else
    assert(!m_nil->red);
    assert(m_nil->maxHigh==LONG_MIN);
#endif
}


/***********************************************************************/
/*  FUNCTION:  DeleteNode */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  returns the Interval stored at deleted node */
/**/
/*    EFFECT:  Deletes z from tree and but don't call destructor */
/*             Then calls FixUpMaxHigh to fix maxHigh fields then calls */
/*             ITDeleteFixUp to restore red-black properties */
/**/
/*    Modifies Input:  z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

template <typename T>
T
IntervalTree<T>::DeleteNode(IntervalTreeNode<T>* z, long& low, long& high)
{
    IntervalTreeNode<T> *x, *y;
    T returnValue = z->data;

    low = z->low;
    high = z->high;

    y= ((z->left == m_nil) || (z->right == m_nil)) ? z : getSuccessor(z);
    x= (y->left == m_nil) ? y->right : y->left;
    if (m_root == (x->parent = y->parent))
        // assignment of y->p to x->p is intentional
        m_root->left=x;
    else
    {
        if (y == y->parent->left)
            y->parent->left=x;
        else
            y->parent->right=x;
    }
    if (y != z) // y should not be nil in this case
    {
        assert(y!=m_nil);
        // y is the node to splice out and x is its child

        y->maxHigh = LONG_MIN;
        y->left=z->left;
        y->right=z->right;
        y->parent=z->parent;
        z->left->parent=z->right->parent=y;
        if (z == z->parent->left)
            z->parent->left=y;
        else
            z->parent->right=y;
        FixUpMaxHigh(x->parent);
        if (!(y->red))
        {
            y->red = z->red;
            DeleteFixUp(x);
        }
        else
            y->red = z->red;
        delete z;
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
        CheckAssumptions();
#else
        assert(!m_nil->red);
        assert(m_nil->maxHigh==LONG_MIN);
#endif
    }
    else
    {
        FixUpMaxHigh(x->parent);
        if (!(y->red))
            DeleteFixUp(x);
        delete y;
#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
        CheckAssumptions();
#else
        assert(!m_nil->red);
        assert(m_nil->maxHigh==LONG_MIN);
#endif
    }
    return returnValue;
}


/***********************************************************************/
/*  FUNCTION:  Enumerate */
/**/
/*    INPUTS:  tree is the tree to look for intervals overlapping the */
/*             closed interval [low,high]  */
/**/
/*    OUTPUT:  stack containing pointers to the nodes overlapping */
/*             [low,high] */
/**/
/*    Modifies Input: none */
/**/
/*    EFFECT:  Returns a stack containing pointers to nodes containing */
/*             intervals which overlap [low,high] in O(max(N,k*log(N))) */
/*             where N is the number of intervals in the tree and k is  */
/*             the number of overlapping intervals                      */
/**/
/*    Note:    This basic idea for this function comes from the  */
/*              _Introduction_To_Algorithms_ book by Cormen et al, but */
/*             modifications were made to return all overlapping intervals */
/*             instead of just the first overlapping interval as in the */
/*             book.  The natural way to do this would require recursive */
/*             calls of a basic search function.  I translated the */
/*             recursive version into an interative version with a stack */
/*             as described below. */
/***********************************************************************/



/* The basic idea for the function below is to take the IntervalSearch
 * function from the book and modify to find all overlapping intervals
 * instead of just one.  This means that any time we take the left
 * branch down the tree we must also check the right branch if and only if
 * we find an overlapping interval in that left branch.  Note this is a
 * recursive condition because if we go left at the root then go left
 * again at the first left child and find an overlap in the left subtree
 * of the left child of root we must recursively check the right subtree
 * of the left child of root as well as the right child of root.
 */
template <typename T>
void
IntervalTree<T>::Enumerate(long low, long high,
    TR1::function<void (IntervalTreeNode<T>*)> callback)
{
    std::vector<RecursionNode> recursionNodeStack(1);

    IntervalTreeNode<T>* x=m_root->left;
    bool stuffToDo = (x != m_nil);
    unsigned int currentParent = 0;

    // Possible speed up: add min field to prune right searches

    while (stuffToDo)
    {
        if (Overlap(low,high,x->low,x->high))
        {
            callback(x);
            recursionNodeStack[currentParent].tryRightBranch=true;
        }
        if(x->left->maxHigh >= low) // implies x != nil
        {
            recursionNodeStack.push_back(RecursionNode(x, currentParent,
                                                       false));
            currentParent =
                static_cast<unsigned int>(recursionNodeStack.size())-1;
            x = x->left;
        }
        else
        {
            x = x->right;
        }
        stuffToDo = (x != m_nil);
        while (!stuffToDo && recursionNodeStack.size() > 1)
        {
            RecursionNode top = recursionNodeStack.back();
            recursionNodeStack.pop_back();
            if (top.tryRightBranch)
            {
                x=top.start_node->right;
                currentParent=top.parentIndex;
                recursionNodeStack[currentParent].tryRightBranch=true;
                stuffToDo = (x != m_nil);
            }
        }
    }
}

#ifdef YASM_INTERVAL_TREE_CHECK_ASSUMPTIONS
template <typename T>
void
IntervalTree<T>::Verify(bool condition, const char* condstr, const char* file,
                        unsigned long line)
{
    if (!condition)
    {
        llvm::errs() << "Assumption \"" << condstr << "\"" << '\n'
                     << "Failed in file " << file << ": at line:" << line
                     << '\n';
        abort();
    }
}
#define YASM_INTERVAL_TREE_VERIFY(condition) \
    Verify(condition, #condition, __FILE__, __LINE__)

template <typename T>
bool
IntervalTree<T>::CheckMaxHighFieldsHelper(IntervalTreeNode<T>* y,
                                          long currentHigh,
                                          bool match) const
{
    if (y != m_nil)
    {
        match = CheckMaxHighFieldsHelper(y->left, currentHigh, match) ?
            true : match;
        YASM_INTERVAL_TREE_VERIFY(y->high <= currentHigh);
        if (y->high == currentHigh)
            match = true;
        match = CheckMaxHighFieldsHelper(y->right, currentHigh, match) ?
            true : match;
    }
    return match;
}

/* Make sure the maxHigh fields for everything makes sense. *
 * If something is wrong, print a warning and exit */
template <typename T>
void
IntervalTree<T>::CheckMaxHighFields(IntervalTreeNode<T>* x) const
{
    if (x != m_nil)
    {
        CheckMaxHighFields(x->left);
        if(!CheckMaxHighFieldsHelper(x, x->maxHigh, false))
        {
            llvm::errs() << "error found in CheckMaxHighFields.\n";
            abort();
        }
        CheckMaxHighFields(x->right);
    }
}

template <typename T>
void
IntervalTree<T>::CheckAssumptions() const
{
    YASM_INTERVAL_TREE_VERIFY(m_nil->low == LONG_MIN);
    YASM_INTERVAL_TREE_VERIFY(m_nil->high == LONG_MIN);
    YASM_INTERVAL_TREE_VERIFY(m_nil->maxHigh == LONG_MIN);
    YASM_INTERVAL_TREE_VERIFY(m_root->low == LONG_MAX);
    YASM_INTERVAL_TREE_VERIFY(m_root->high == LONG_MAX);
    YASM_INTERVAL_TREE_VERIFY(m_root->maxHigh == LONG_MAX);
    YASM_INTERVAL_TREE_VERIFY(m_nil->data == NULL);
    YASM_INTERVAL_TREE_VERIFY(m_root->data == NULL);
    YASM_INTERVAL_TREE_VERIFY(m_nil->red == 0);
    YASM_INTERVAL_TREE_VERIFY(m_root->red == 0);
    CheckMaxHighFields(m_root->left);
}
#endif

} // namespace yasm

#endif
